#ifndef OPENFMOD_STUDIO_AUTOMATION_RESOLVER_HPP
#define OPENFMOD_STUDIO_AUTOMATION_RESOLVER_HPP
#include "OpenFmod/Studio/Automation/Controller.hpp"
#include "OpenFmod/Studio/Modulation/Mapping.hpp"
namespace OpenFmod::Studio {
enum class Resolution { Minimum, Maximum, Sum, Average, Multiply, Replace };
enum class AutomationType { Normal, Volume };
struct ModulationValue {
    float Value{};
    bool IsEnvelope{};
};
class AutomationResolver {
public:
    AutomationResolver(Resolution resolution, AutomationType type, const Mapping* mapping = nullptr);
    float Resolve(float base, const std::vector<AutomationValue>& values) const;
    float Modulate(float base, const std::vector<ModulationValue>& values) const;
private:
    float initial() const;
    float combine(float accumulated, float value) const;
    float finish(float accumulated, std::size_t count, float base) const;
    Resolution resolution;
    AutomationType type;
    const Mapping* mapping;
};
}
#endif
