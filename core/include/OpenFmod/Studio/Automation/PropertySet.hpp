#ifndef OPENFMOD_STUDIO_AUTOMATION_PROPERTYSET_HPP
#define OPENFMOD_STUDIO_AUTOMATION_PROPERTYSET_HPP
#include "OpenFmod/Studio/Automation/Instance.hpp"
namespace OpenFmod::Studio {
class PropertySet {
public:
    PropertySet(const Library& library, std::map<std::uint32_t, float> base, const std::map<std::uint32_t, Property>& properties, const AutomationBindings& bindings, const std::function<float()>& random);
    void Update(const AutomationInput& input, double milliseconds);
    float Get(std::uint32_t id) const;
    void SetBase(std::uint32_t id, float value);
    void Release(double milliseconds);
    bool IsFinished(double milliseconds) const;
    void ProcessAudio(std::uint32_t property, const Bank::Guid& modulator, const float* samples, std::size_t count, double milliseconds);
private:
    std::map<std::uint32_t, float> base;
    std::map<std::uint32_t, float> values;
    std::map<std::uint32_t, PropertyInstance> instances;
};
}
#endif
