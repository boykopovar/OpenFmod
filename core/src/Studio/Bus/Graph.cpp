#include "OpenFmod/Studio/Bus/Graph.hpp"
#include "OpenFmod/Studio/Levels.hpp"
#include <cmath>
namespace OpenFmod::Studio {
BusGraph::BusGraph(const Library& library, std::uint32_t sampleRate)
    : library(library), sampleRate(sampleRate), graph(sampleRate), effectBindings(library, Event{}) {
    Refresh();
}
BusGraph::Key BusGraph::key(const Bank::Guid& bus, std::uint64_t scope) const {
    if (!scope) return {0, bus};
    const auto found = scopes.find(scope);
    if (found == scopes.end()) throw FormatError("Unknown bus scope");
    if (found->second.Local.count(bus)) return {scope, bus};
    return key(bus, found->second.Parent);
}
MixControls BusGraph::baseLevels(const Bus& bus, const std::optional<MixControls>& animated) const {
    const auto& initial = animated ? *animated : bus.Levels;
    MixControls levels{initial.VolumeDecibels, initial.PitchSemitones, {}};
    snapshots.ValidateTarget(bus.Id, [](auto property) { return property <= 1; });
    levels.VolumeDecibels = snapshotProperty(bus.Id, 0, levels.VolumeDecibels, bus.Properties, AutomationType::Volume);
    levels.PitchSemitones = snapshotProperty(bus.Id, 1, levels.PitchSemitones, bus.Properties, AutomationType::Normal);
    struct Visit { const Bank::Guid& Id; const Visit* Parent; };
    const auto apply = [&](const auto& self, const Bank::Guid& id, const Visit* parent) -> void {
        for (auto previous = parent; previous; previous = previous->Parent) {
            if (previous->Id == id) throw FormatError("Cyclic volume control routing");
        }
        const Visit visit{id, parent};
        const auto& control = library.Require<VolumeControl>(id);
        snapshots.ValidateTarget(id, [](auto property) { return property <= 1; });
        levels.VolumeDecibels += snapshotProperty(id, 0, control.Levels.VolumeDecibels, control.Properties, AutomationType::Volume);
        levels.PitchSemitones += snapshotProperty(id, 1, control.Levels.PitchSemitones, control.Properties, AutomationType::Normal);
        for (const auto& ancestor : control.Levels.VolumeControls) self(self, ancestor, &visit);
    };
    for (const auto& control : bus.Levels.VolumeControls) apply(apply, control, nullptr);
    return levels;
}
std::uint64_t BusGraph::resolve(const Bank::Guid& bus, std::uint64_t scope) {
    const auto identity = key(bus, scope);
    if (const auto found = nodes.find(identity); found != nodes.end()) return found->second.AudioId;
    if (!resolving.insert(identity).second) throw FormatError("Cyclic bus routing");
    try {
        const auto model = library.Retain<Bus>(bus);
        auto parent = model->Routing.Bus;
        auto parentScope = identity.first;
        if (identity.first && scopes.at(identity.first).Master == bus) {
            const auto& owner = scopes.at(identity.first);
            parent = owner.Input;
            parentScope = owner.Parent;
        }
        const auto parentId = parent == Bank::Guid{} ? 0 : resolve(parent, parentScope);
        Node node;
        node.Model = model;
        node.ParentAudioId = parentId;
        const auto levels = baseLevels(*model);
        node.BaseGain = VolumeGain(levels.VolumeDecibels);
        node.BasePitch = PitchRatio(levels.PitchSemitones);
        node.AudioId = graph.AddBus(parentId, static_cast<Audio::SpeakerMode>(model->ChannelLayout));
        try {
            graph.SetGain(node.AudioId, node.BaseGain);
            graph.SetPitch(node.AudioId, node.BasePitch);
            attach(node, model->PreEffects, Audio::ProcessingStage::BeforeFader);
            attach(node, model->PostEffects, Audio::ProcessingStage::AfterFader);
            applySnapshots(node);
            nodes.emplace(identity, node);
            connectionsDirty = true;
        } catch (...) {
            graph.RemoveBus(node.AudioId);
            throw;
        }
        resolving.erase(identity);
        return node.AudioId;
    } catch (...) {
        resolving.erase(identity);
        throw;
    }
}
std::uint64_t BusGraph::Resolve(const Bank::Guid& bus, std::uint64_t scope) {
    const auto result = resolve(bus, scope);
    connect();
    return result;
}
Audio::Mixer& BusGraph::GetMixer(const Bank::Guid& bus, std::uint64_t scope) { return graph.GetMixer(Resolve(bus, scope)); }
void BusGraph::SetVolume(const Bank::Guid& bus, float volume, std::uint64_t scope) {
    if (!std::isfinite(volume) || volume < 0) throw FormatError("Invalid bus volume");
    const auto id = Resolve(bus, scope);
    auto& node = nodes.at(key(bus, scope));
    graph.SetGain(id, node.BaseGain * volume);
    node.UserGain = volume;
}
void BusGraph::Render(float* stereo, std::size_t frames) {
    if (frames) updateEffects(frames);
    graph.Render(stereo, frames);
}
void BusGraph::SetPitch(const Bank::Guid& bus, float pitch, std::uint64_t scope) {
    if (!std::isfinite(pitch) || pitch <= 0) throw FormatError("Invalid bus pitch");
    const auto id = Resolve(bus, scope);
    auto& node = nodes.at(key(bus, scope));
    graph.SetPitch(id, node.BasePitch * pitch);
    node.UserPitch = pitch;
}
std::optional<std::uint64_t> BusGraph::FramesUntilNextEnd() const { return graph.FramesUntilNextEnd(); }
}
