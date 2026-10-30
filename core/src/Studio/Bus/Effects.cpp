#include "OpenFmod/Studio/Bus/Graph.hpp"
#include "OpenFmod/Studio/Levels.hpp"
namespace OpenFmod::Studio {
void BusGraph::attach(Node& node, const std::vector<BusEffect>& effects, Audio::ProcessingStage stage) {
    for (const auto& item : effects) {
        if (library.Contains<Effect>(item.Id)) {
            const auto& model = library.Require<Effect>(item.Id);
            auto processor = std::make_shared<EffectProcessor>(model, sampleRate);
            if (!node.Effects.emplace(item.Id, processor).second) throw FormatError("Duplicate bus effect");
            graph.AddProcessor(node.AudioId, stage, std::move(processor));
        } else {
            const auto retained = library.Retain<Sidechain>(item.Id);
            node.Sidechains.emplace(item.Id, retained);
            const auto& model = *retained;
            const auto gain = VolumeGain(model.GainDecibels);
            auto tap = std::make_shared<Audio::SignalTap>();
            tap->SetGain(gain);
            if (!node.Taps.emplace(item.Id, tap).second) throw FormatError("Duplicate sidechain tap");
            graph.AddProcessor(node.AudioId, stage, std::move(tap));
        }
    }
}
void BusGraph::connect() {
    std::size_t previous;
    do {
        previous = nodes.size();
        for (const auto& [identity, node] : nodes) {
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
}
}
