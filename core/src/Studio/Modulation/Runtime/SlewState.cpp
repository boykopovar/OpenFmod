#include "OpenFmod/Studio/Modulation/Runtime/SlewState.hpp"
#include <cmath>
namespace OpenFmod::Studio {
SlewState::SlewState(float ascendingSpeed, float descendingSpeed, const Mapping* mapping, bool discrete)
    : mapping(mapping), ascendingSpeed(ascendingSpeed), descendingSpeed(descendingSpeed), discrete(discrete) {
    if (!std::isfinite(ascendingSpeed) || !std::isfinite(descendingSpeed) || ascendingSpeed < 0 || descendingSpeed < 0) {
        throw FormatError("Invalid slew speed");
    }
}
void SlewState::Reset(float value) {
    if (!std::isfinite(value)) throw FormatError("Nonfinite slew value");
    this->value = value;
    remainder = 0;
    initialized = true;
}
float SlewState::GetValue() const {
    if (!initialized) throw FormatError("Slew state is not initialized");
    return value;
}
float SlewState::Process(float target, double seconds) {
    if (!std::isfinite(target) || !std::isfinite(seconds) || seconds < 0) throw FormatError("Invalid slew input");
    if (!initialized) Reset(target);
    if (target == value || seconds == 0) return value;
    const float current = value + remainder;
    const float origin = mapping ? mapping->Inverse(current) : current;
    const float destination = mapping ? mapping->Inverse(target) : target;
    const float distance = destination - origin;
    const float speed = distance > 0 ? ascendingSpeed : descendingSpeed;
    if (speed == 0) {
        Reset(target);
        return value;
    }
    const double step = speed * seconds * (mapping ? mapping->GetInputRange() : 1);
    if (step >= std::abs(distance)) {
        Reset(target);
        return value;
    }
    const auto next = static_cast<float>(origin + std::copysign(step, distance));
    const float result = mapping ? mapping->Transform(next) : next;
    if (!std::isfinite(result)) throw FormatError("Slew result overflow");
    if (discrete) {
        float integer;
        remainder = std::modf(result - value, &integer);
        value += integer;
    } else {
        value = result;
    }
    return value;
}
}
