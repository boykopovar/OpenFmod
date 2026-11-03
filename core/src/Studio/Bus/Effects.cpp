#include "OpenFmod/Studio/Bus/Graph.hpp"
#include "OpenFmod/Studio/Levels.hpp"
namespace OpenFmod::Studio {
void BusGraph::attach(Node& node, const std::vector<BusEffect>& effects, Audio::ProcessingStage stage) {
    for (const auto& item : effects) {
        if (library.Contains<Effect>(item.Id)) {
            const auto retained = library.Retain<Effect>(item.Id);
            node.EffectModels.emplace(item.Id, retained);
            const auto& model = *retained;
            auto processor = std::make_shared<EffectProcessor>(model, sampleRate);
            if (!model.Properties.empty()) {
                std::map<std::uint32_t, float> base;
                for (std::size_t index = 0; index < model.Parameters.size(); ++index) {
                    if (const auto value = std::get_if<float>(&model.Parameters[index])) base.emplace(static_cast<std::uint32_t>(1000 + index), *value);
                }
                node.EffectProperties.emplace(item.Id, std::make_shared<PropertySet>(library, std::move(base), model.Properties, effectBindings, std::function<float()>{}));
            }
            if (!node.Effects.emplace(item.Id, processor).second) throw FormatError("Duplicate bus effect");
            graph.AddTransform(node.AudioId, stage, std::move(processor));
        } else {
            const auto retained = library.Retain<Sidechain>(item.Id);
            node.Sidechains.emplace(item.Id, retained);
            const auto& model = *retained;
            const auto gain = VolumeGain(model.GainDecibels);
            auto tap = std::make_shared<Audio::SignalTap>();
            tap->SetGain(gain);
            if (!node.Taps.emplace(item.Id, tap).second) throw FormatError("Duplicate sidechain tap");
            graph.AddTransform(node.AudioId, stage, std::move(tap));
        }
    }
}
void BusGraph::connect() {
    Refresh();
    if (!connectionsDirty) return;
    std::size_t previous;
    do {
        previous = nodes.size();
        for (const auto& [identity, node] : nodes) {
            for (const auto& [id, effect] : node.EffectModels) for (const auto& [propertyId, property] : effect->Properties) {
                for (const auto& modulator : property.Modulators) for (const auto& bus : sourceBuses(modulator)) resolve(bus, identity.first);
                static_cast<void>(id);
                static_cast<void>(propertyId);
            }
            for (const auto& [id, processor] : node.Effects) {
                const auto sources = controls.find(id);
                if (sources == controls.end()) continue;
                for (const auto& control : sources->second) {
                    const auto found = providers.find(control);
                    if (found == providers.end()) throw FormatError("Sidechain has no source bus");
                    for (const auto& bus : found->second) resolve(bus, identity.first);
                }
                static_cast<void>(processor);
            }
        }
    } while (nodes.size() != previous);
    for (const auto& [identity, node] : nodes) {
        for (const auto& [id, processor] : node.Effects) {
            const auto sources = controls.find(id);
            if (sources == controls.end()) continue;
            std::vector<std::shared_ptr<Audio::SignalTap>> taps;
            for (const auto& control : sources->second) {
                for (const auto& bus : providers.at(control)) {
                    const auto& source = nodes.at(key(bus, identity.first));
                    taps.push_back(source.Taps.at(control));
                    const auto edge = std::make_pair(source.AudioId, node.AudioId);
                    if (edge.first != edge.second && !connections.count(edge)) {
                        graph.AddDependency(edge.first, edge.second);
                        connections.insert(edge);
                    }
                }
            }
            processor->SetSidechains(std::move(taps));
        }
    }
    connectionsDirty = false;
}
}
