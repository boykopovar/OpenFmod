#include "OpenFmod/Studio/Automation/Resolver.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
namespace OpenFmod::Studio {
AutomationResolver::AutomationResolver(Resolution resolution, AutomationType type, const Mapping* mapping)
    : resolution(resolution), type(type), mapping(mapping) {
    if (resolution < Resolution::Minimum || resolution > Resolution::Replace
        || type < AutomationType::Normal || type > AutomationType::Volume) throw FormatError("Invalid automation resolution");
}
float AutomationResolver::initial() const {
    if (resolution == Resolution::Minimum) return std::numeric_limits<float>::max();
    if (resolution == Resolution::Maximum) return std::numeric_limits<float>::lowest();
    if (resolution == Resolution::Multiply) return 1;
    return 0;
}
float AutomationResolver::combine(float accumulated, float value) const {
    switch (resolution) {
        case Resolution::Minimum: return std::min(accumulated, value);
        case Resolution::Maximum: return std::max(accumulated, value);
        case Resolution::Average: return accumulated + value;
        case Resolution::Multiply: return accumulated * value;
        case Resolution::Replace: return value;
        case Resolution::Sum:
            if (type == AutomationType::Volume) {
                if (accumulated == std::numeric_limits<float>::lowest() || value <= -80) return std::numeric_limits<float>::lowest();
                return accumulated + value;
            }
            return accumulated + value - (mapping ? mapping->Inverse(0) : 0);
    }
    throw FormatError("Invalid automation resolution");
}
float AutomationResolver::finish(float accumulated, std::size_t count, float base) const {
    if (!count) return type == AutomationType::Normal && mapping ? mapping->Inverse(base) : base;
    if (resolution == Resolution::Average) return accumulated / static_cast<float>(count);
    if (resolution == Resolution::Sum) {
        if (type == AutomationType::Volume) return std::max(accumulated, -80.0f);
        return accumulated + (mapping ? mapping->Inverse(0) : 0);
    }
    return accumulated;
}
float AutomationResolver::Resolve(float base, const std::vector<AutomationValue>& values) const {
    if (!std::isfinite(base)) throw FormatError("Nonfinite automation base value");
    float accumulated = initial();
    std::size_t count = 0;
    std::array<AutomationValue, 2> partial{};
    std::size_t partialCount = 0;
    for (const auto& entry : values) {
        if (!std::isfinite(entry.Value) || !std::isfinite(entry.Weight) || entry.Weight < 0 || entry.Weight > 1) {
            throw FormatError("Invalid automation contribution");
        }
        if (entry.Weight == 0) continue;
        const auto value = type == AutomationType::Volume && mapping ? mapping->Transform(entry.Value) : entry.Value;
        if (entry.Weight == 1) {
            accumulated = combine(accumulated, value);
            ++count;
            for (std::size_t index = 0; index < partialCount; ++index) partial[index].Value = combine(partial[index].Value, value);
        } else {
            if (partialCount == partial.size()) throw FormatError("Too many simultaneous partial automation contributions");
            partial[partialCount++] = {combine(accumulated, value), entry.Weight};
        }
    }
    auto result = finish(accumulated, count, base);
    for (std::size_t index = 0; index < partialCount; ++index) {
        const auto target = finish(partial[index].Value, count + 1, base);
        result += (target - result) * partial[index].Weight;
    }
    if (!std::isfinite(result)) throw FormatError("Automation result overflow");
    if (type == AutomationType::Normal && mapping) result = mapping->Transform(result);
    return result;
}
float AutomationResolver::Modulate(float base, const std::vector<ModulationValue>& values) const {
    if (!std::isfinite(base)) throw FormatError("Nonfinite modulation base value");
    auto sum = type == AutomationType::Normal && mapping ? mapping->Inverse(base) : base;
    float multiplier = 1;
    for (const auto& entry : values) {
        if (!std::isfinite(entry.Value)) throw FormatError("Nonfinite modulation contribution");
        auto value = entry.Value;
        if (type == AutomationType::Volume) {
            if (mapping) value = mapping->Transform(value);
            sum = sum == std::numeric_limits<float>::lowest() || value <= -80
                ? std::numeric_limits<float>::lowest() : sum + value;
        } else {
            if (mapping) value *= mapping->GetInputRange() * 0.01f;
            if (entry.IsEnvelope && resolution == Resolution::Multiply) multiplier *= value;
            else sum += value;
        }
    }
    auto result = multiplier * sum;
    if (!std::isfinite(result)) throw FormatError("Modulation result overflow");
    if (type == AutomationType::Normal && mapping) result = mapping->Transform(result);
    return result;
}
}
