#include "OpenFmod/Studio/Snapshot/Mix.hpp"
#include <cmath>
namespace OpenFmod::Studio {
bool SnapshotContribution::operator==(const SnapshotContribution& other) const {
    return Priority == other.Priority && Value == other.Value && Intensity == other.Intensity && Additive == other.Additive && Resolution == other.Resolution;
}
bool SnapshotMix::SetLayers(const std::vector<SnapshotLayer>& layers) {
    decltype(values) updated;
    for (const auto& layer : layers) {
        const auto& value = layer.Contribution;
        if (!std::isfinite(value.Value) || !std::isfinite(value.Intensity) || value.Intensity < 0 || value.Intensity > 100
            || (value.Resolution != SnapshotResolution::Average && value.Resolution != SnapshotResolution::Replace)) throw FormatError("Invalid snapshot mix layer");
        updated[layer.Target][layer.Property].push_back(value);
    }
    if (values == updated) return false;
    values.swap(updated);
    return true;
}
float SnapshotMix::Resolve(const Bank::Guid& target, std::uint32_t property, float base, AutomationType type, const Mapping* mapping) const {
    const auto found = values.find(target);
    if (found == values.end()) return base;
    const auto value = found->second.find(property);
    if (value == found->second.end()) return base;
    return SnapshotResolver(type, mapping).Resolve(base, value->second);
}
void SnapshotMix::ValidateTarget(const Bank::Guid& target, const std::function<bool(std::uint32_t)>& accepts) const {
    if (!accepts) throw FormatError("Missing snapshot target validator");
    const auto found = values.find(target);
    if (found == values.end()) return;
    for (const auto& [property, contributions] : found->second) {
        if (!accepts(property)) throw FormatError("Unknown snapshot target property: " + std::to_string(property));
        static_cast<void>(contributions);
    }
}
}
