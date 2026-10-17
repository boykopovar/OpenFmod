#include "OpenFmod/Studio/Snapshot/Resolver.hpp"
#include <algorithm>
#include <cmath>
namespace OpenFmod::Studio {
SnapshotResolver::SnapshotResolver(AutomationType type, const Mapping* mapping) : type(type), mapping(mapping) {
    if (type != AutomationType::Normal && type != AutomationType::Volume) throw FormatError("Invalid snapshot property type");
}
float SnapshotResolver::toLinear(float value) const {
    if (!std::isfinite(value)) throw FormatError("Nonfinite snapshot value");
    if (type != AutomationType::Volume) return value;
    if (mapping) value = mapping->Transform(value);
    return value <= -80 ? 0 : std::pow(10.0f, value * 0.05f);
}
float SnapshotResolver::fromLinear(float value) const {
    if (!std::isfinite(value)) throw FormatError("Snapshot result overflow");
    if (type != AutomationType::Volume) return value;
    value = value > 0 ? 20 * std::log10(value) : -80;
    return mapping ? mapping->Inverse(value) : value;
}
float SnapshotResolver::Resolve(float base, const std::vector<SnapshotContribution>& values) const {
    float result = toLinear(base);
    std::vector<const SnapshotContribution*> sorted;
    sorted.reserve(values.size());
    for (const auto& value : values) {
        if (!std::isfinite(value.Intensity) || value.Intensity < 0 || value.Intensity > 100
            || !std::isfinite(value.Value) || (value.Resolution != SnapshotResolution::Average
            && value.Resolution != SnapshotResolution::Replace)) throw FormatError("Invalid snapshot contribution");
        if (value.Intensity > 0) sorted.push_back(&value);
    }
    std::stable_sort(sorted.begin(), sorted.end(), [](const auto* a, const auto* b) { return a->Priority < b->Priority; });
    std::size_t begin = 0;
    while (begin < sorted.size()) {
        auto end = begin + 1;
        while (end < sorted.size() && sorted[end]->Priority == sorted[begin]->Priority) ++end;
        const bool additive = sorted[begin]->Additive && type == AutomationType::Volume;
        const auto apply = [&](float value, float weight) {
            result += ((additive ? result * value : value) - result) * weight;
        };
        float sum = 0;
        float total = 0;
        float maximum = 0;
        for (auto index = begin; index < end; ++index) {
            const auto& value = *sorted[index];
            const float weight = value.Intensity * 0.01f;
            const float target = toLinear(value.Value);
            if (value.Resolution == SnapshotResolution::Replace) apply(target, weight);
            else {
                sum += target * weight;
                total += weight;
                maximum = std::max(maximum, weight);
            }
        }
        if (total > 0) apply(sum / total, maximum);
        begin = end;
    }
    return fromLinear(result);
}
}
