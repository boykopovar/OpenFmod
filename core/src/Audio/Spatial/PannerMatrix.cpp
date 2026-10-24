#include "OpenFmod/Audio/Spatial/Panner.hpp"
#include "OpenFmod/Binary/Reader.hpp"
#include <algorithm>
#include <cmath>
namespace OpenFmod::Audio {
namespace {
void validate(const PannerSettings& settings) {
    const float values[]{settings.Balance, settings.Direction, settings.Extent, settings.Rotation,
        settings.LowFrequencyDecibels, settings.Separation, settings.Axis, settings.MinimumDistance,
        settings.MaximumDistance, settings.SoundSize, settings.MinimumExtent, settings.SpatialBlend, settings.HeightBlend};
    for (const auto value : values) if (!std::isfinite(value)) throw FormatError("Nonfinite panner parameter");
    if (settings.Mode < 0 || settings.Mode > 2 || settings.Balance < -100 || settings.Balance > 100
        || settings.Extent < 0 || settings.Extent > 360 || settings.Separation < 0 || settings.Separation > 360
        || settings.MinimumDistance < 0 || settings.MaximumDistance < 0 || settings.SoundSize < 0
        || settings.MinimumExtent < 0 || settings.MinimumExtent > 360 || settings.SpatialBlend < 0 || settings.SpatialBlend > 1
        || settings.HeightBlend < -1 || settings.HeightBlend > 1 || settings.Listeners.empty() || settings.Listeners.size() > 8) {
        throw FormatError("Invalid panner settings");
    }
    if (settings.EnabledSpeakers != 4095) throw FormatError("Restricted speaker layouts are not implemented");
    for (const auto& listener : settings.Listeners) {
        const float components[]{listener.Weight, listener.Direction, listener.Rotation, listener.Distance, listener.HorizontalDistance};
        for (const auto value : components) if (!std::isfinite(value)) throw FormatError("Nonfinite listener parameter");
        if (listener.Weight < 0 || listener.Distance < 0 || listener.HorizontalDistance < 0) throw FormatError("Invalid listener parameters");
    }
}
void add(std::vector<float>& output, const std::vector<float>& input, float gain) {
    for (std::size_t index = 0; index < output.size(); ++index) output[index] += input[index] * gain;
}
}
std::array<float, 4> PannerMatrix(const PannerSettings& settings) {
    validate(settings);
    if (settings.Mode == 0) return {0.5f, 0.5f, 0.5f, 0.5f};
    if (settings.Mode == 1) {
        const auto gain = StereoBalance(settings.Balance);
        return {gain[0], 0, 0, gain[1]};
    }
    const Speakers speakers(settings.Layout);
    const auto channels = speakers.GetChannels();
    std::vector<float> matrix(channels * 2);
    if (settings.SpatialBlend < 1) {
        std::vector<float> flat;
        if (settings.Discrete) {
            flat = speakers.Project(settings.Axis - settings.Separation * 0.5f, 0);
            const auto right = speakers.Project(settings.Axis + settings.Separation * 0.5f, 0);
            flat.insert(flat.end(), right.begin(), right.end());
        } else flat = speakers.ProjectStereo(settings.Direction, settings.Extent, settings.Rotation);
        add(matrix, flat, 1 - settings.SpatialBlend);
    }
    if (settings.SpatialBlend > 0) {
        auto listeners = settings.Listeners;
        std::stable_sort(listeners.begin(), listeners.end(), [](const auto& first, const auto& second) { return first.Distance < second.Distance; });
        std::vector<float> spatial(channels * 2);
        float weight = 0;
        float attenuation = 0;
        for (const auto& listener : listeners) {
            const auto gain = DistanceAttenuation(settings.Attenuation, listener.Distance, settings.MinimumDistance, settings.MaximumDistance);
            const auto extent = AngularExtent(settings.ExtentControl, listener.HorizontalDistance, settings.MinimumDistance, settings.SoundSize, settings.MinimumExtent);
            const auto contribution = listeners.size() == 1 ? 1 : listener.Weight;
            const auto used = std::min(1 - weight, contribution);
            attenuation += gain * used;
            weight += used;
            add(spatial, speakers.ProjectStereo(listener.Direction, extent, listener.Rotation), contribution * gain);
        }
        float gain = settings.SpatialBlend;
        if (listeners.size() > 1) {
            float power = 0;
            for (const auto value : spatial) power += value * value;
            gain *= power > 0 ? attenuation / std::sqrt(power) : 0;
        }
        add(matrix, spatial, gain);
    }
    if (channels >= 6 && settings.LowFrequencyUpmix) {
        const auto low = settings.LowFrequencyDecibels <= -80 ? 0 : std::pow(10.0f, settings.LowFrequencyDecibels * 0.05f);
        matrix[3] = matrix[channels + 3] = low * 0.70710677f;
    }
    const auto downmix = speakers.StereoDownmix();
    std::array<float, 4> result{};
    for (std::size_t input = 0; input < 2; ++input) {
        for (std::size_t output = 0; output < 2; ++output) {
            for (std::size_t channel = 0; channel < channels; ++channel) {
                result[input * 2 + output] += matrix[input * channels + channel] * downmix[output * channels + channel];
            }
        }
    }
    return result;
}
}
