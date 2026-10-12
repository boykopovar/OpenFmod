#ifndef OPENFMOD_STUDIO_MODULATION_RANDOMSTATE_HPP
#define OPENFMOD_STUDIO_MODULATION_RANDOMSTATE_HPP
#include "OpenFmod/Studio/Modulation/Mapping.hpp"
#include <optional>
namespace OpenFmod::Studio {
enum class RandomRange { Symmetric, Negative };
class RandomState {
public:
    RandomState(float amount, RandomRange range, const Mapping* mapping = nullptr, bool discrete = false, bool angular = false);
    void Start(float random);
    void SetAmount(float amount);
    float GetValue() const;
private:
    void setRange(float amount);
    float evaluate() const;
    RandomRange range;
    const Mapping* mapping;
    bool discrete;
    bool angular;
    float minimum{};
    float maximum{};
    std::optional<float> random;
};
}
#endif
