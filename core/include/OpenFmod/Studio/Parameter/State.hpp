#ifndef OPENFMOD_STUDIO_PARAMETER_STATE_HPP
#define OPENFMOD_STUDIO_PARAMETER_STATE_HPP
#include "OpenFmod/Studio/Parameter/Model.hpp"
#include "OpenFmod/Studio/Modulation/SlewState.hpp"
#include <memory>
namespace OpenFmod::Studio {
class ParameterState {
public:
    ParameterState(std::shared_ptr<const Parameter> model, float initialValue);
    const Parameter& GetModel() const;
    bool IsGlobal() const;
    bool IsReadOnly() const;
    float GetTarget() const;
    float GetValue() const;
    void SetValue(float value, bool immediate = false);
    void SetAutomaticValue(float value, bool immediate = false);
    void Advance(double seconds);
private:
    void assign(float value, bool immediate);
    std::shared_ptr<const Parameter> model;
    SlewState slew;
    float target{};
};
}
#endif
