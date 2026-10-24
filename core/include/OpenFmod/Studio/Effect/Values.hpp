#ifndef OPENFMOD_STUDIO_EFFECT_VALUES_HPP
#define OPENFMOD_STUDIO_EFFECT_VALUES_HPP
#include "OpenFmod/Studio/Effect/Model.hpp"
namespace OpenFmod::Studio {
class EffectValues {
public:
    EffectValues(const std::vector<EffectParameter>& values, std::size_t count) : values(values) {
        if (values.size() != count) throw FormatError("Incorrect effect parameter count");
    }
    template<class TValue>
    TValue Get(std::size_t index) const {
        const auto value = std::get_if<TValue>(&values.at(index));
        if (!value) throw FormatError("Incorrect effect parameter type at index " + std::to_string(index));
        return *value;
    }
private:
    const std::vector<EffectParameter>& values;
};
}
#endif
