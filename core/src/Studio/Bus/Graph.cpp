#include "OpenFmod/Studio/Bus/Graph.hpp"
#include "OpenFmod/Studio/Levels.hpp"
#include <cmath>
namespace OpenFmod::Studio {
BusGraph::BusGraph(const Library& library, std::uint32_t sampleRate)
    : library(library), sampleRate(sampleRate), graph(sampleRate) {
    for (const auto& bus : library.GetAll<Bus>()) {
        for (const auto* chain : {&bus->PreEffects, &bus->PostEffects}) {
            for (const auto& effect : *chain) {
                if (library.Contains<Sidechain>(effect.Id)) providers[effect.Id].push_back(bus->Id);
            }
        }
    }
    for (const auto& control : library.GetAll<Sidechain>()) {
        for (const auto& effect : control->Effects) controls[effect].push_back(control->Id);
    }
}
BusGraph::Key BusGraph::key(const Bank::Guid& bus, std::uint64_t scope) const {
    if (!scope) return {0, bus};
    const auto found = scopes.find(scope);
    if (found == scopes.end()) throw FormatError("Unknown bus scope");
    return {found->second.Local.count(bus) ? scope : 0, bus};
}
MixControls BusGraph::baseLevels(const Bus& bus) const {
    auto levels = bus.Levels;
    std::set<Bank::Guid> visiting;
    std::function<void(const Bank::Guid&)> apply = [&](const Bank::Guid& id) {
        if (!visiting.insert(id).second) throw FormatError("Cyclic volume control routing");
        const auto& control = library.Require<VolumeControl>(id);
        levels.VolumeDecibels += control.Levels.VolumeDecibels;
        levels.PitchSemitones += control.Levels.PitchSemitones;
        for (const auto& parent : control.Levels.VolumeControls) apply(parent);
        visiting.erase(id);
    };
    for (const auto& control : bus.Levels.VolumeControls) apply(control);
    return levels;
}
std::uint64_t BusGraph::resolve(const Bank::Guid& bus, std::uint64_t scope) {
    const auto identity = key(bus, scope);
    if (const auto found = nodes.find(identity); found != nodes.end()) return found->second.AudioId;
    if (!resolving.insert(identity).second) throw FormatError("Cyclic bus routing");
    try {
        const auto model = library.Retain<Bus>(bus);
        auto parent = model->Routing.Bus;
        if (identity.first && scopes.at(identity.first).Master == bus) parent = scopes.at(identity.first).Input;
        const auto parentId = parent == Bank::Guid{} ? 0 : resolve(parent, scope);
        Node node;
        node.Model = model;
        const auto levels = baseLevels(*model);
        node.BaseGain = VolumeGain(levels.VolumeDecibels);
        node.BasePitch = PitchRatio(levels.PitchSemitones);
        node.AudioId = graph.AddBus(parentId);
        try {
            graph.SetGain(node.AudioId, node.BaseGain);
            graph.SetPitch(node.AudioId, node.BasePitch);
            attach(node, model->PreEffects, Audio::ProcessingStage::BeforeFader);
            attach(node, model->PostEffects, Audio::ProcessingStage::AfterFader);
            nodes.emplace(identity, node);
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
    graph.SetGain(id, nodes.at(key(bus, scope)).BaseGain * volume);
}
void BusGraph::Render(float* stereo, std::size_t frames) { graph.Render(stereo, frames); }
void BusGraph::SetPitch(const Bank::Guid& bus, float pitch, std::uint64_t scope) {
    if (!std::isfinite(pitch) || pitch <= 0) throw FormatError("Invalid bus pitch");
    const auto id = Resolve(bus, scope);
    graph.SetPitch(id, nodes.at(key(bus, scope)).BasePitch * pitch);
}
std::optional<std::uint64_t> BusGraph::FramesUntilNextEnd() const { return graph.FramesUntilNextEnd(); }
}
