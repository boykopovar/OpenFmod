#include "OpenFmod/Audio/Spatial/Projection.hpp"
#include "OpenFmod/Audio/Spatial/Layout.hpp"
#include "OpenFmod/Audio/Spatial/Speakers.hpp"
#include "OpenFmod/Audio/Spatial/Panner.hpp"
#include <cmath>
#include <stdexcept>
void CheckSpatial() {
    using namespace OpenFmod::Audio;
    PlanarPanner panner({4, {{0, -45}, {1, 45}, {2, -135}, {3, 135}}, {}});
    const auto diffuse = panner.Project(0, 360);
    for (const auto gain : diffuse) if (gain != 0.5f) throw std::runtime_error("Unequal diffuse speaker gains");
    for (int angle = -180; angle < 180; angle += 5) {
        for (int extent = 0; extent <= 360; extent += 15) {
            const auto gains = panner.Project(static_cast<float>(angle), static_cast<float>(extent));
            float power = 0;
            for (const auto gain : gains) {
                if (gain < 0) throw std::runtime_error("Negative planar speaker gain");
                power += gain * gain;
            }
            if (std::abs(power - 1) > 0.000001f) throw std::runtime_error("Planar projection changed power");
        }
    }
    const auto single = panner.Project(-45, 0);
    if (single[0] != 1 || single[1] != 0 || single[2] != 0 || single[3] != 0) {
        throw std::runtime_error("Source on a speaker leaked into another channel");
    }
    const Speakers speakers(SpeakerMode::Surround51);
    for (int rotation = -180; rotation <= 180; rotation += 30) {
        const auto matrix = speakers.ProjectStereo(25, 210, static_cast<float>(rotation));
        for (std::size_t channel = 0; channel < 2; ++channel) {
            float power = 0;
            for (std::size_t speaker = 0; speaker < speakers.GetChannels(); ++speaker) {
                const auto gain = matrix[channel * speakers.GetChannels() + speaker];
                power += gain * gain;
            }
            if (std::abs(power - 1) > 0.000001f) throw std::runtime_error("Rotated stereo source changed power");
        }
    }
    PannerSettings settings;
    settings.Mode = 2;
    settings.Layout = SpeakerMode::Surround51;
    settings.SpatialBlend = 1;
    settings.Listeners[0].Distance = 100;
    const auto muted = PannerMatrix(settings);
    for (const auto value : muted) if (value != 0) throw std::runtime_error("Distant source was not attenuated");
    settings.Mode = 1;
    Panner whole(settings), partitioned(settings);
    settings.Balance = 100;
    whole.Configure(settings, 64);
    partitioned.Configure(settings, 64);
    std::vector<float> first(256, 1), second(first);
    whole.Process(first.data(), 128);
    partitioned.Process(second.data(), 23);
    partitioned.Process(second.data() + 46, 105);
    if (first != second) throw std::runtime_error("Panning ramp depends on block partitioning");
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
