#include "OpenFmod/Audio/Spatial/Projection.hpp"
#include <cmath>
#include <stdexcept>
void CheckSpatial() {
    using namespace OpenFmod::Audio;
    const auto balance = StereoBalance(0);
    if (std::abs(balance[0] - 1) > 0.000001f || std::abs(balance[1] - 1) > 0.000001f) {
        throw std::runtime_error("Centered balance changes channel gain");
    }
    for (int angle = -180; angle <= 180; angle += 15) {
        for (int extent = 0; extent <= 360; extent += 45) {
            const auto left = StereoProjection(static_cast<float>(angle), static_cast<float>(extent));
            const auto right = StereoProjection(static_cast<float>(-angle), static_cast<float>(extent));
            if (std::abs(left[0] - right[1]) > 0.000001f || std::abs(left[0] * left[0] + left[1] * left[1] - 1) > 0.000001f) {
                throw std::runtime_error("Spatial projection lost symmetry or power");
            }
        }
    }
    if (DistanceAttenuation(Rolloff::Linear, 6, 2, 10) != 0.5f
        || DistanceAttenuation(Rolloff::LinearSquared, 6, 2, 10) != 0.25f
        || DistanceAttenuation(Rolloff::Inverse, 20, 2, 10) != 0.2f
        || DistanceAttenuation(Rolloff::InverseTapered, 20, 2, 10) != 0) {
        throw std::runtime_error("Incorrect distance attenuation");
    }
    if (AngularExtent(ExtentMode::Automatic, 0, 1, 0, 0) != 360
        || AngularExtent(ExtentMode::Off, 0, 1, 0, 360) != 0
        || AngularExtent(ExtentMode::User, 100, 1, 2, 60) != 60) {
        throw std::runtime_error("Incorrect spatial source extent");
    }
}
