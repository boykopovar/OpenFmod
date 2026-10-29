#ifndef OPENFMOD_STUDIO_MODULATION_SLEWSTATE_HPP
#define OPENFMOD_STUDIO_MODULATION_SLEWSTATE_HPP
#include "OpenFmod/Studio/Modulation/Mapping.hpp"
namespace OpenFmod::Studio {
class SlewState {
public:
    SlewState(float ascendingSpeed, float descendingSpeed, const Mapping* mapping = nullptr, bool discrete = false);
    void Reset(float value);
    float Process(float target, double seconds);
    float GetValue() const;
private:
    const Mapping* mapping;
    float ascendingSpeed;
    float descendingSpeed;
    float value{};
    float remainder{};
    bool discrete;
    bool initialized{};
};
}
#endif
