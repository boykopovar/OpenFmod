#include "OpenFmod/Studio/Effect/Configuration.hpp"
#include "OpenFmod/Studio/Effect/Values.hpp"
#include <cmath>
namespace OpenFmod::Studio {
namespace {
std::vector<Audio::SpatialListener> readListeners(const std::string& bytes) {
    if (bytes.empty()) return {Audio::SpatialListener{}};
    if (bytes.size() != 468) throw FormatError("Incorrect spatial attributes size");
    Binary::Reader reader(bytes);
    const auto count = reader.Unsigned(4);
    if (!count || count > 8) throw FormatError("Invalid listener count");
    std::array<Audio::SpatialListener, 8> listeners;
    for (auto& listener : listeners) {
        std::array<float, 12> attributes;
        for (auto& value : attributes) {
            value = reader.Float();
            if (!std::isfinite(value)) throw FormatError("Invalid spatial attribute");
        }
        const auto x = attributes[0];
        const auto y = attributes[1];
        const auto z = attributes[2];
        listener.Direction = std::atan2(x, z) * 57.295776f;
        listener.Rotation = std::atan2(attributes[6], attributes[8]) * 57.295776f;
        listener.HorizontalDistance = std::sqrt(x * x + z * z);
        listener.Distance = std::sqrt(x * x + y * y + z * z);
    }
    for (auto& listener : listeners) listener.Weight = reader.Float();
    for (std::size_t index = 0; index < 12; ++index) {
        if (!std::isfinite(reader.Float())) throw FormatError("Invalid absolute spatial attribute");
    }
    reader.Finish();
    return {listeners.begin(), listeners.begin() + count};
}
}
Audio::PannerSettings PannerConfiguration(const std::vector<EffectParameter>& values) {
    const EffectValues parameters(values, 24);
    Audio::PannerSettings result;
    result.Mode = parameters.Get<std::int32_t>(0);
    result.Balance = parameters.Get<float>(1);
    result.Direction = parameters.Get<float>(2);
    result.Extent = parameters.Get<float>(3);
    result.Rotation = parameters.Get<float>(4);
    result.LowFrequencyDecibels = parameters.Get<float>(5);
    const auto stereoMode = parameters.Get<std::int32_t>(6);
    if (stereoMode < 0 || stereoMode > 1) throw FormatError("Invalid stereo panning mode");
    result.Discrete = stereoMode != 0;
    result.Separation = parameters.Get<float>(7);
    result.Axis = parameters.Get<float>(8);
    result.EnabledSpeakers = static_cast<std::uint32_t>(parameters.Get<std::int32_t>(9));
    result.Listeners = readListeners(parameters.Get<std::string>(10));
    result.Attenuation = static_cast<Audio::Rolloff>(parameters.Get<std::int32_t>(11));
    result.MinimumDistance = parameters.Get<float>(12);
    result.MaximumDistance = parameters.Get<float>(13);
    result.ExtentControl = static_cast<Audio::ExtentMode>(parameters.Get<std::int32_t>(14));
    result.SoundSize = parameters.Get<float>(15);
    result.MinimumExtent = parameters.Get<float>(16);
    result.SpatialBlend = parameters.Get<float>(17);
    const auto lowFrequencyUpmix = parameters.Get<std::int32_t>(18);
    if (lowFrequencyUpmix < 0 || lowFrequencyUpmix > 1) throw FormatError("Invalid low frequency upmix mode");
    result.LowFrequencyUpmix = lowFrequencyUpmix != 0;
    const auto overall = parameters.Get<std::string>(19);
    if (!overall.empty() && overall.size() != 8) throw FormatError("Invalid overall gain data");
    result.Layout = static_cast<Audio::SpeakerMode>(parameters.Get<std::int32_t>(20));
    result.HeightBlend = parameters.Get<float>(21);
    const auto range = parameters.Get<std::string>(22);
    const auto overrideRange = parameters.Get<bool>(23);
    if (!range.empty()) {
        Binary::Reader reader(range);
        const auto minimum = reader.Float();
        const auto maximum = reader.Float();
        reader.Finish();
        if (!std::isfinite(minimum) || !std::isfinite(maximum) || minimum < 0 || maximum < 0) throw FormatError("Invalid attenuation range");
        if (!overrideRange) {
            result.MinimumDistance = minimum;
            result.MaximumDistance = maximum;
        }
    }
    return result;
}
}
