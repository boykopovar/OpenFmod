#include "OpenFmod/Studio/Automation/Property.hpp"
namespace OpenFmod::Studio {
PropertyAutomation::PropertyAutomation(const Library& library, Property property, const AutomationBindings& bindings)
    : property(std::move(property)), bindings(bindings),
      mapping(this->property.MappingId == Bank::Guid{} ? nullptr : library.Retain<Mapping>(this->property.MappingId)),
      resolver(static_cast<Resolution>(this->property.Resolution), static_cast<AutomationType>(this->property.ValueType), mapping.get()) {
    values.reserve(this->property.Curves.size());
}
float PropertyAutomation::Evaluate(float base, const AutomationInput& input, const std::vector<ModulationValue>& modulation) {
    if (modulation.size() != property.Modulators.size()) throw FormatError("Property modulation count mismatch");
    values.clear();
    for (const auto& curve : property.Curves) values.push_back(bindings.Evaluate(curve, input.Tick, input.Parameters, input.ControllerBlends ? input.ControllerBlends(curve) : input.Blend));
    return resolver.Modulate(resolver.Resolve(base, values), modulation);
}
const Property& PropertyAutomation::GetModel() const { return property; }
}
