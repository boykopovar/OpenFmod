#ifndef OPENFMOD_STUDIO_PLAYBACK_CONTROLS_HPP
#define OPENFMOD_STUDIO_PLAYBACK_CONTROLS_HPP
#include "OpenFmod/Studio/Automation/PropertySet.hpp"
namespace OpenFmod::Studio {
class InstrumentControls {
public:
    InstrumentControls(const Library& library, const Instrument& model, const AutomationBindings& bindings, const std::function<float()>& random);
    void Update(const AutomationInput& input, double milliseconds);
    float Get(std::uint32_t id) const;
    float GetGain() const;
    float GetPitch(const AutomationInput& input) const;
    void Release(double milliseconds);
    bool IsFinished(double milliseconds) const;
private:
    static std::map<std::uint32_t, float> baseProperties(const Instrument& model);
    PropertySet properties;
    Bank::Guid autoPitchParameter;
    float parameterMinimum{};
    float autoPitchMaximum{};
    float autoPitchMinimum{};
};
}
#endif
