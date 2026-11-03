#include "OpenFmod/Studio/Bus/Graph.hpp"
#include <algorithm>
namespace OpenFmod::Studio {
std::vector<Bank::Guid> BusGraph::sourceBuses(const Bank::Guid& target) const {
    std::vector<Bank::Guid> result;
    const auto sources = controls.find(target);
    if (sources == controls.end()) return result;
    for (const auto& control : sources->second) {
        const auto found = providers.find(control);
        if (found == providers.end()) throw FormatError("Modulator sidechain has no source bus");
        result.insert(result.end(), found->second.begin(), found->second.end());
    }
    return result;
}
void BusGraph::updateEffects(std::size_t frames) {
    const auto milliseconds = static_cast<double>(frames) * 1000 / sampleRate;
    effectMilliseconds += milliseconds;
    for (auto& [identity, node] : nodes) {
        if (node.EffectProperties.empty()) continue;
        for (auto& [id, properties] : node.EffectProperties) {
            for (const auto& [propertyId, property] : node.EffectModels.at(id)->Properties) {
                for (const auto& modulator : property.Modulators) {
                    const auto& model = library.Require<PropertyModel<Modulation>>(modulator).Definition;
                    if (!std::holds_alternative<EnvelopeFollower>(model.Parameters)) continue;
                    Audio::SignalMeter meter;
                    const auto sources = controls.find(modulator);
                    if (sources != controls.end()) for (const auto& control : sources->second) {
                        for (const auto& bus : providers.at(control)) {
                            const auto value = nodes.at(key(bus, identity.first)).Taps.at(control)->GetMeter();
                            meter.Peak = std::max(meter.Peak, value.Peak);
                            meter.Energy += value.Energy;
                        }
                    }
                    properties->ProcessMeter(propertyId, modulator, meter.Peak, meter.Energy, milliseconds);
                }
            }
            properties->Update({}, effectMilliseconds);
        }
        applySnapshots(node);
    }
}
}
