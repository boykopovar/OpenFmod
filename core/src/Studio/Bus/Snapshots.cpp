#include "OpenFmod/Studio/Bus/Graph.hpp"
#include "OpenFmod/Studio/Levels.hpp"
namespace OpenFmod::Studio {
float BusGraph::snapshotProperty(const Bank::Guid& target, std::uint32_t id, float base, const std::map<std::uint32_t, Property>& properties, AutomationType type) const {
    const auto property = properties.find(id);
    const Mapping* mapping = nullptr;
    if (property != properties.end()) {
        type = static_cast<AutomationType>(property->second.ValueType);
        if (property->second.MappingId != Bank::Guid{}) mapping = &library.Require<Mapping>(property->second.MappingId);
    }
    return snapshots.Resolve(target, id, base, type, mapping);
}
void BusGraph::applySnapshots(Node& node) {
    const auto levels = baseLevels(*node.Model);
    node.BaseGain = VolumeGain(levels.VolumeDecibels);
    node.BasePitch = PitchRatio(levels.PitchSemitones);
    graph.SetGain(node.AudioId, node.BaseGain * node.UserGain);
    graph.SetPitch(node.AudioId, node.BasePitch * node.UserPitch);
    for (const auto& [id, effect] : node.EffectModels) {
        snapshots.ValidateTarget(id, [&](auto property) {
            return property >= 1000 && property - 1000 < effect->Parameters.size()
                && std::holds_alternative<float>(effect->Parameters[property - 1000]);
        });
        for (std::size_t i = 0; i < effect->Parameters.size(); ++i) {
            const auto base = std::get_if<float>(&effect->Parameters[i]);
            if (!base) continue;
            const auto value = snapshotProperty(id, static_cast<std::uint32_t>(1000 + i), *base, effect->Properties, AutomationType::Normal);
            node.Effects.at(id)->SetParameter(i, value);
        }
    }
    for (const auto& [id, sidechain] : node.Sidechains) {
        snapshots.ValidateTarget(id, [](auto property) { return property == 0; });
        const auto value = snapshotProperty(id, 0, sidechain->GainDecibels, sidechain->Properties, AutomationType::Volume);
        node.Taps.at(id)->SetGain(VolumeGain(value));
    }
}
void BusGraph::SetSnapshots(const std::vector<SnapshotLayer>& layers) {
    snapshots.SetLayers(layers);
    for (auto& [key, node] : nodes) {
        applySnapshots(node);
        static_cast<void>(key);
    }
}
}
