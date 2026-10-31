#include "OpenFmod/Studio/Playback/Controls.hpp"
#include "OpenFmod/Studio/Levels.hpp"
#include <algorithm>
#include <cmath>
namespace OpenFmod::Studio {
std::map<std::uint32_t, float> InstrumentControls::baseProperties(const Instrument& model) {
    auto base = model.Settings.Properties;
    base.emplace(4, 1);
    base.emplace(5, 0);
    base.emplace(6, 0);
    if (const auto* silence = std::get_if<SilenceInstrument>(&model.Definition.Source)) base.emplace(29, silence->DurationMilliseconds);
    if (const auto* event = std::get_if<EventInstrument>(&model.Definition.Source)) {
        base.emplace(3, event->SnapshotIntensity);
        for (const auto& parameter : event->Parameters) {
            if (parameter.PropertyId < 1000 || !base.emplace(parameter.PropertyId, parameter.Value).second) throw FormatError("Invalid nested event parameter property");
        }
    }
    if (const auto* command = std::get_if<CommandInstrument>(&model.Definition.Source)) base.emplace(2, command->Value);
    return base;
}
InstrumentControls::InstrumentControls(const Library& library, const Instrument& model, const AutomationBindings& bindings, const std::function<float()>& random)
    : properties(library, baseProperties(model), model.Properties, bindings, random), autoPitchParameter(model.Settings.AutoPitchParameter),
      autoPitchMaximum(model.Settings.AutoPitchMaximum), autoPitchMinimum(model.Settings.AutoPitchMinimum) {
    if (autoPitchParameter != Bank::Guid{}) parameterMinimum = library.Require<Parameter>(autoPitchParameter).Minimum;
    for (const auto& [id, property] : model.Properties) {
        for (const auto& modulator : property.Modulators) {
            if (std::holds_alternative<Envelope>(library.Require<PropertyModel<Modulation>>(modulator).Definition.Parameters)) envelope = true;
        }
        static_cast<void>(id);
    }
}
void InstrumentControls::Update(const AutomationInput& input, double milliseconds) { properties.Update(input, milliseconds); }
float InstrumentControls::Get(std::uint32_t id) const { return properties.Get(id); }
float InstrumentControls::GetGain() const { return VolumeGain(properties.Get(0)) * properties.Get(4); }
float InstrumentControls::GetPitch(const AutomationInput& input) const {
    float multiplier = 1;
    if (autoPitchParameter != Bank::Guid{}) {
        if (!input.Parameters) throw FormatError("Missing automatic pitch parameter input");
        const auto range = autoPitchMaximum - parameterMinimum;
        if (range != 0) multiplier = autoPitchMinimum
            + (input.Parameters(autoPitchParameter) - parameterMinimum) / range * (1 - autoPitchMinimum);
        if (!std::isfinite(multiplier) || multiplier < 0) throw FormatError("Invalid automatic pitch multiplier");
    }
    return multiplier * PitchRatio(std::clamp(properties.Get(1), -48.0f, 48.0f));
}
void InstrumentControls::Release(double milliseconds) { properties.Release(milliseconds); }
bool InstrumentControls::IsFinished(double milliseconds) const { return properties.IsFinished(milliseconds); }
bool InstrumentControls::HasEnvelope() const { return envelope; }
}
