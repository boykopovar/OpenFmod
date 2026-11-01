#ifndef OPENFMOD_STUDIO_RUNTIME_STATE_HPP
#define OPENFMOD_STUDIO_RUNTIME_STATE_HPP
#include "OpenFmod/Studio/Runtime/Context.hpp"
#include "OpenFmod/Studio/Event/Playback/Instruments.hpp"
#include "OpenFmod/Studio/Snapshot/Instance.hpp"
#include "OpenFmod/Studio/Timeline/Timing/TempoGrid.hpp"
#include <deque>
namespace OpenFmod::Studio {
class EventState : public IEventControl {
public:
    EventState(std::shared_ptr<RuntimeContext> context, const Bank::Guid& event, std::shared_ptr<EventRouting> parent, std::optional<Bank::Guid> input);
    void Start(const SoundStart& position);
    void Stop(bool immediate);
    void End(bool cutOff);
    void Prepare(std::uint32_t ticks);
    void Update();
    std::optional<std::uint64_t> NextBoundary() const;
    AutomationInput Input() const;
    void StartEvent(const Bank::Guid& event) override;
    void StopEvents(const Bank::Guid& event, bool immediate) override;
    float GetParameter(const Bank::Guid& parameter) const override;
    const Parameter& GetParameterModel(const Bank::Guid& parameter) const override;
    void SetParameter(const Bank::Guid& parameter, float value) override;
    Bank::Guid BankId{};
    bool Available{true};
    std::shared_ptr<RuntimeContext> Context;
    std::shared_ptr<const PropertyModel<Event>> Model;
    std::shared_ptr<EventRouting> Routing;
    ParameterScope Parameters;
    AutomationBindings Bindings;
    PlaylistState Playlists;
    std::shared_ptr<const TimelineResource> Timeline;
    std::unique_ptr<TimelineTransport> Transport;
    std::unique_ptr<TimelineSchedule> Schedule;
    std::unique_ptr<SnapshotInstance> Snapshot;
    std::unique_ptr<InstrumentFactory> Factory;
    std::unique_ptr<EventInstruments> Instruments;
    PlaybackState State{PlaybackState::Stopped};
    std::uint64_t LaunchOrder{};
    bool Oneshot{};
    float Gain{1};
    float Pitch{1};
    float Intensity{100};
    std::uint32_t Position{};
private:
    struct Action { std::uint64_t Tick; TimelineAction Value; };
    struct Span { std::uint64_t Tick; TimelineSpan Value; };
    std::deque<Action> actions;
    std::vector<Span> spans;
    std::uint64_t started{};
    std::uint64_t updated{};
    std::uint32_t duration{};
    std::uint32_t plannedPosition{};
    std::uint64_t plannedUntil{};
    bool ending{};
};
}
#endif
