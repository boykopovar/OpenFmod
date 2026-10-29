#include "OpenFmod/Studio/Automation/Instance.hpp"
#include <algorithm>
namespace OpenFmod::Studio {
PropertyInstance::PropertyInstance(const Library& library, Property property, const AutomationBindings& bindings, const std::function<float()>& random, bool discrete)
    : automation(library, std::move(property), bindings) {
    const auto& settings = automation.GetModel();
    auto mapping = settings.MappingId == Bank::Guid{} ? nullptr : library.Retain<Mapping>(settings.MappingId);
    modulators.reserve(settings.Modulators.size());
    values.reserve(settings.Modulators.size());
    for (const auto& id : settings.Modulators) {
        const auto& model = library.Require<PropertyModel<Modulation>>(id);
        if (!model.Properties.empty()) throw FormatError("Modulator has animated properties");
        float sample = 0;
        if (std::holds_alternative<RandomModulation>(model.Definition.Parameters)) {
            if (!random) throw FormatError("Missing modulation random source");
            sample = random();
        }
        modulators.emplace_back(model.Definition, mapping, discrete, settings.Id == 6, sample);
    }
}
float PropertyInstance::Evaluate(float base, const AutomationInput& input, double milliseconds) {
    values.clear();
    for (const auto& modulator : modulators) values.push_back(modulator.Evaluate(milliseconds));
    return automation.Evaluate(base, input, values);
}
void PropertyInstance::ProcessAudio(const Bank::Guid& modulator, const float* samples, std::size_t count, double milliseconds) {
    const auto& ids = automation.GetModel().Modulators;
    const auto found = std::find(ids.begin(), ids.end(), modulator);
    if (found == ids.end()) throw FormatError("Modulator does not belong to property");
    modulators[static_cast<std::size_t>(found - ids.begin())].ProcessAudio(samples, count, milliseconds);
}
void PropertyInstance::Release(double milliseconds) {
    for (auto& modulator : modulators) modulator.Release(milliseconds);
}
bool PropertyInstance::IsFinished(double milliseconds) const {
    return std::all_of(modulators.begin(), modulators.end(), [milliseconds](const auto& modulator) { return modulator.IsFinished(milliseconds); });
}
}
