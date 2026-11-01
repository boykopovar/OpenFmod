#include "OpenFmod/Studio/Runtime/State.hpp"
#include "OpenFmod/Studio/Event/Inspection.hpp"
#include "OpenFmod/Studio/Timeline/Playback/Bounds.hpp"
#include <cmath>
#include <limits>
namespace OpenFmod::Studio {
EventRouting::EventRouting(BusGraph& graph, const Event& event, std::shared_ptr<EventRouting> parent, std::optional<Bank::Guid> input)
    : Graph(graph), Parent(std::move(parent)), Scope(graph.CreateScope(event, Parent ? Parent->Scope : 0, input)) {}
EventRouting::~EventRouting() { Graph.RemoveScope(Scope); }
EventState::EventState(std::shared_ptr<RuntimeContext> context, const Bank::Guid& event, std::shared_ptr<EventRouting> parent, std::optional<Bank::Guid> input)
    : Context(std::move(context)), Model(Context->Resources.Retain<PropertyModel<Event>>(event)),
      Parameters(Context->Resources, Model->Definition, Context->Globals), Bindings(Context->Resources, Model->Definition),
      Playlists(Context->Resources, Context->Playlists) {
    BankId = Context->Resources.GetOwner<PropertyModel<Event>>(event)->Id;
    const auto& definition = Model->Definition;
    if (!Model->Properties.empty()) throw FormatError("Event level animated properties require runtime bindings");
    if (definition.MasterBus != Bank::Guid{}) Routing = std::make_shared<EventRouting>(Context->Buses, definition, std::move(parent), input);
    if (definition.Timeline != Bank::Guid{}) {
        Timeline = Context->Resources.Retain<TimelineResource>(definition.Timeline);
        duration = TimelineDuration(*Timeline);
    }
    Oneshot = IsEventOneshot(Context->Resources, event);
    Factory = std::make_unique<InstrumentFactory>(Context->Resources, Context->Sources, Bindings, Playlists,
        [this] { return Context->Random(); }, Context->SampleRate,
        [this](const auto& bus) -> Audio::Mixer& {
            if (!Routing) throw FormatError("Audio instrument belongs to an event without a master bus");
            return Context->Buses.GetMixer(bus, Routing->Scope);
        },
        [this](const auto& child, const auto& bus) { return Context->Create(child, Routing, bus); }, *this,
        [this](const auto& name) {
            if (!Context->Programmer) throw FormatError("Programmer sound provider is missing");
            return Context->Programmer(name);
        });
}
AutomationInput EventState::Input() const { return {Position, [this](const auto& id) { return Parameters.GetValue(id); }, {}}; }
void EventState::Start(const SoundStart& position) {
    if (!Available) throw FormatError("Event bank has been unloaded");
    if (!std::isfinite(position.OffsetPercent) || position.OffsetPercent < 0 || position.OffsetPercent > 100 || position.Repeats < -1) throw FormatError("Invalid event start position");
    Stop(true);
    actions.clear();
    spans.clear();
    ending = false;
    started = updated = Context->Clock.GetTick();
    plannedUntil = started;
    const auto offset = static_cast<double>(position.OffsetTicks) + static_cast<double>(duration) * position.OffsetPercent / 100;
    if (offset > std::numeric_limits<std::uint32_t>::max()) throw FormatError("Event timeline offset overflow");
    Position = plannedPosition = static_cast<std::uint32_t>(offset);
    if (Timeline) {
        Transport = std::make_unique<TimelineTransport>(*Timeline);
        Transport->Seek(Position);
        Schedule = std::make_unique<TimelineSchedule>(*Timeline);
    }
    Instruments = std::make_unique<EventInstruments>(Context->Resources, Bindings, *Factory, [this] { return Context->Random(); },
        [this](auto tick, auto mode, auto interval) {
            if (!mode) return tick;
            if (!Timeline) throw FormatError("Instrument quantization requires an event timeline");
            const auto now = Context->Clock.GetTick();
            if (tick < now) throw FormatError("Quantized event time is in the past");
            const auto target = static_cast<std::uint64_t>(Position) + tick - now;
            return tick + TempoGrid(Timeline->Definition.Tempo).Quantize(target, static_cast<QuantizationMode>(mode), interval) - target;
        });
    if (Model->Definition.Snapshot != Bank::Guid{}) {
        Snapshot = std::make_unique<SnapshotInstance>(Context->Resources, Model->Definition.Snapshot, Bindings, [this] { return Context->Random(); });
        Snapshot->SetIntensity(Intensity);
    }
    initializeMix();
    State = PlaybackState::Playing;
    std::size_t index = 0;
    for (const auto& instrument : Model->Definition.Instruments) Instruments->Activate({0, index++, false}, instrument, 0, Input(), started);
    Update();
}
void EventState::Stop(bool immediate) {
    if (State == PlaybackState::Stopped) return;
    const auto tick = Context->Clock.GetTick();
    Instruments->Stop(immediate, tick);
    for (auto& entry : mix) entry.second->Release(static_cast<double>(tick - started) / 48);
    if (Snapshot) Snapshot->Stop(immediate, static_cast<double>(tick - started) / 48);
    actions.clear();
    State = immediate ? PlaybackState::Stopped : PlaybackState::Stopping;
    if (immediate) { Instruments.reset(); Snapshot.reset(); }
}
void EventState::End(bool cutOff) {
    if (cutOff) Stop(false);
    else {
        ending = true;
        if (Transport) { Transport->SetStopping(true); Transport->KeyOff(); }
    }
}
}
