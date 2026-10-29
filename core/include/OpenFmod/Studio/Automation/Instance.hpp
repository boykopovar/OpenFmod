#ifndef OPENFMOD_STUDIO_AUTOMATION_INSTANCE_HPP
#define OPENFMOD_STUDIO_AUTOMATION_INSTANCE_HPP
#include "OpenFmod/Studio/Automation/Property.hpp"
#include "OpenFmod/Studio/Modulation/Runtime/Instance.hpp"
namespace OpenFmod::Studio {
class PropertyInstance {
public:
    PropertyInstance(const Library& library, Property property, const AutomationBindings& bindings, const std::function<float()>& random, bool discrete = false);
    float Evaluate(float base, const AutomationInput& input, double milliseconds);
    void ProcessAudio(const Bank::Guid& modulator, const float* samples, std::size_t count, double milliseconds);
    void Release(double milliseconds);
    bool IsFinished(double milliseconds) const;
private:
    PropertyAutomation automation;
    std::vector<ModulationInstance> modulators;
    std::vector<ModulationValue> values;
};
}
#endif
