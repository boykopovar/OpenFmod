#include "OpenFmod/Audio/Spatial/Projection.hpp"
#include "OpenFmod/Binary/Reader.hpp"
#include <algorithm>
#include <cmath>
namespace OpenFmod::Audio {
namespace {
void requireRange(float value, float minimum, float maximum) {
    if (!std::isfinite(value) || value < minimum || value > maximum) throw FormatError("Invalid spatial audio parameter");
}
}
std::array<float, 2> StereoBalance(float position) {
    requireRange(position, -100, 100);
    return {std::cos(position * 0.007853982038795948f + 0.7853981852531433f) * 1.414213538f,
        std::cos((100 - position) * 0.007853982038795948f) * 1.414213538f};
}
std::array<float, 2> StereoProjection(float directionDegrees, float extentDegrees) {
    if (!std::isfinite(directionDegrees)) throw FormatError("Invalid sound direction");
    requireRange(extentDegrees, 0, 360);
    const auto extent = extentDegrees * 0.0087266471f;
    const auto width = extentDegrees < 0.00001f ? 1 : std::sin(extent) / extent;
    const auto direction = std::sin(std::remainder(directionDegrees, 360.0f) * 0.0174532925f) * width;
    const auto energy = std::sqrt(std::max(0.0f, 2 - direction * direction));
    return {(energy - direction) * 0.5f, (energy + direction) * 0.5f};
}
float DistanceAttenuation(Rolloff mode, float distance, float minimum, float maximum, float scale) {
    if (!std::isfinite(distance) || !std::isfinite(minimum) || !std::isfinite(maximum) || !std::isfinite(scale)
        || distance < 0 || minimum < 0 || maximum < 0 || scale < 0) throw FormatError("Invalid attenuation range");
    if (mode < Rolloff::LinearSquared || mode > Rolloff::Custom) throw FormatError("Unknown distance rolloff");
    minimum = std::min(minimum, maximum);
    if (distance <= minimum || scale == 0 || mode == Rolloff::Custom) return 1;
    const auto scaled = (distance - minimum) * scale + minimum;
    if (!std::isfinite(scaled)) throw FormatError("Distance attenuation overflow");
    if (mode == Rolloff::Inverse) {
        if (maximum == 0) throw FormatError("Inverse attenuation has an empty range");
        return minimum / std::min(scaled, maximum);
    }
    if (scaled >= maximum) return 0;
    const auto linear = (maximum - scaled) / (maximum - minimum);
    if (mode == Rolloff::Linear) return linear;
    if (mode == Rolloff::LinearSquared) return linear * linear;
    return std::min(minimum / scaled, linear * linear);
}
float AngularExtent(ExtentMode mode, float horizontalDistance, float minimumDistance, float soundSize, float minimumExtent) {
    if (!std::isfinite(horizontalDistance) || !std::isfinite(minimumDistance) || !std::isfinite(soundSize)
        || horizontalDistance < 0 || minimumDistance < 0 || soundSize < 0) throw FormatError("Invalid sound extent");
    requireRange(minimumExtent, 0, 360);
    if (mode == ExtentMode::Off) return 0;
    if (mode != ExtentMode::Automatic && mode != ExtentMode::User) throw FormatError("Unknown sound extent mode");
    const auto size = mode == ExtentMode::Automatic ? minimumDistance * 2 : soundSize;
    float extent = 0;
    if (size > 0 && horizontalDistance < size * 5) {
        const auto ratio = horizontalDistance / size;
        extent = 360 / (ratio * ratio * 4 + 1);
    }
    return mode == ExtentMode::User ? std::max(extent, minimumExtent) : extent;
}
}
