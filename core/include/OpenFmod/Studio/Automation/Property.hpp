#ifndef OPENFMOD_STUDIO_AUTOMATION_PROPERTY_HPP
#define OPENFMOD_STUDIO_AUTOMATION_PROPERTY_HPP
#include "OpenFmod/Studio/Automation/Bindings.hpp"
#include "OpenFmod/Studio/Automation/Resolver.hpp"
namespace OpenFmod::Studio {
struct AutomationInput {
    std::uint32_t Tick{};
    std::function<float(const Bank::Guid&)> Parameters;
    AutomationBlend Blend;
    std::function<AutomationBlend(const Bank::Guid&)> ControllerBlends;
};
class PropertyAutomation {
public:
    PropertyAutomation(const Library& library, Property property, const AutomationBindings& bindings);
    float Evaluate(float base, const AutomationInput& input, const std::vector<ModulationValue>& modulation);
    const Property& GetModel() const;
private:
    Property property;
    const AutomationBindings& bindings;
    std::shared_ptr<const Mapping> mapping;
    AutomationResolver resolver;
    std::vector<AutomationValue> values;
};
}
#endif
