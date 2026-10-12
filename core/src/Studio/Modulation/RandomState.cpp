#include "OpenFmod/Studio/Modulation/RandomState.hpp"
#include <cmath>
namespace OpenFmod::Studio {
RandomState::RandomState(float amount, RandomRange range, const Mapping* mapping, bool discrete, bool angular)
    : range(range), mapping(mapping), discrete(discrete), angular(angular) {
    if (range != RandomRange::Symmetric && range != RandomRange::Negative) throw FormatError("Invalid random modulation range");
    if (discrete && (!mapping || mapping->GetOutputRange() <= 0)) throw FormatError("Discrete random modulation requires a nonempty output range");
    setRange(amount);
}
void RandomState::setRange(float amount) {
    if (!std::isfinite(amount) || amount < 0) throw FormatError("Invalid random modulation amount");
    float low;
    float high;
    if (range == RandomRange::Symmetric) {
        low = -amount * 0.5f;
        if (discrete) {
            const auto width = mapping->GetOutputRange();
            high = ((width + 1) * amount) / width + low;
        } else {
            high = amount * 0.5f;
        }
    } else {
        low = mapping ? mapping->Inverse(-amount) : -amount;
        high = mapping ? mapping->Inverse(0) : 0;
    }
    if (!std::isfinite(low) || !std::isfinite(high)) throw FormatError("Random modulation range overflow");
    minimum = low;
    maximum = high;
}
void RandomState::Start(float random) {
    if (!std::isfinite(random) || random < 0 || random > 1) throw FormatError("Invalid random modulation sample");
    this->random = random;
}
void RandomState::SetAmount(float amount) { setRange(amount); }
float RandomState::GetValue() const {
    if (!random) throw FormatError("Random modulation has not started");
    return evaluate();
}
float RandomState::evaluate() const {
    if (!angular) return (maximum - minimum) * *random + minimum;
    const auto low = std::sin(minimum * 0.03141592815518379f);
    const auto high = std::sin(maximum * 0.03141592815518379f);
    const auto value = (high - low) * *random + low;
    if (value < -1 || value > 1) throw FormatError("Angular random modulation overflow");
    return std::asin(value) * 31.83098793029785f;
}
}
