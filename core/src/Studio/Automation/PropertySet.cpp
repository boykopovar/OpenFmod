#include "OpenFmod/Studio/Automation/PropertySet.hpp"
#include <algorithm>
#include <cmath>
namespace OpenFmod::Studio {
PropertySet::PropertySet(const Library& library, std::map<std::uint32_t, float> base, const std::map<std::uint32_t, Property>& properties, const AutomationBindings& bindings, const std::function<float()>& random)
    : base(std::move(base)), values(this->base) {
    for (const auto& [id, value] : this->base) {
        if (!std::isfinite(value)) throw FormatError("Nonfinite base property");
        static_cast<void>(id);
    }
    for (const auto& [id, property] : properties) {
        if (id != property.Id || !this->base.count(id)) throw FormatError("Missing base value for automated property");
        instances.try_emplace(id, library, property, bindings, random);
    }
}
void PropertySet::Update(const AutomationInput& input, double milliseconds) {
    for (const auto& [id, value] : base) {
        const auto instance = instances.find(id);
        const auto result = instance == instances.end() ? value : instance->second.Evaluate(value, input, milliseconds);
        if (!std::isfinite(result)) throw FormatError("Nonfinite automated property");
        values.at(id) = result;
    }
}
float PropertySet::Get(std::uint32_t id) const {
    const auto found = values.find(id);
    if (found == values.end()) throw FormatError("Unknown playback property");
    return found->second;
}
void PropertySet::SetBase(std::uint32_t id, float value) {
    if (!std::isfinite(value)) throw FormatError("Nonfinite base property");
    const auto found = base.find(id);
    if (found == base.end()) throw FormatError("Unknown playback property");
    found->second = value;
}
void PropertySet::Release(double milliseconds) {
    for (auto& [id, instance] : instances) {
        instance.Release(milliseconds);
        static_cast<void>(id);
    }
}
bool PropertySet::IsFinished(double milliseconds) const {
    return std::all_of(instances.begin(), instances.end(), [milliseconds](const auto& entry) { return entry.second.IsFinished(milliseconds); });
}
PropertyInstance& PropertySet::requireInstance(std::uint32_t property) {
    const auto found = instances.find(property);
    if (found == instances.end()) throw FormatError("Unknown automated property audio input");
    return found->second;
}
void PropertySet::ProcessAudio(std::uint32_t property, const Bank::Guid& modulator, const float* samples, std::size_t count, double milliseconds) {
    requireInstance(property).ProcessAudio(modulator, samples, count, milliseconds);
}
void PropertySet::ProcessMeter(std::uint32_t property, const Bank::Guid& modulator, float peak, float energy, double milliseconds) {
    requireInstance(property).ProcessMeter(modulator, peak, energy, milliseconds);
}

}
