#include "OpenFmod/Audio/Filters/MultibandEq.hpp"
#include "OpenFmod/Binary/Reader.hpp"
#include <cmath>
#include <limits>
namespace OpenFmod::Audio {
MultibandEq::MultibandEq(std::uint32_t sampleRate) : sampleRate(sampleRate) {
    if (sampleRate < 8000) throw FormatError("Invalid multiband equalizer sample rate");
}
void MultibandEq::Configure(std::size_t index, EqualizerBand settings, std::uint32_t rampFrames) {
    if (index >= bands.size()) throw FormatError("Invalid equalizer band index");
    const auto coefficients = DesignEqualizerFilter(sampleRate, settings);
    auto& band = bands[index];
    if (band.Settings.Type != settings.Type) rampFrames = 0;
    band.Settings = settings;
    band.Stages = FilterStages(settings.Type);
    for (std::size_t stage = 0; stage < band.Stages; ++stage) band.Filters[stage].Configure(coefficients, rampFrames);
}
void MultibandEq::Process(float* stereo, std::size_t frames) { Process(stereo, frames, 2); }
void MultibandEq::Process(float* stereo, std::size_t frames, std::size_t channels) {
    if (!channels || channels > 8 || frames > std::numeric_limits<std::size_t>::max() / channels || (frames && !stereo)) {
        throw FormatError("Invalid multiband equalizer buffer");
    }
    for (auto& band : bands) {
        if (!band.Stages) continue;
        for (std::size_t frame = 0; frame < frames; ++frame) {
            std::array<float, 8> value{};
            for (std::size_t channel = 0; channel < channels; ++channel) value[channel] = stereo[frame * channels + channel];
            for (std::size_t stage = 0; stage < band.Stages; ++stage) band.Filters[stage].Process(value.data(), channels);
            for (std::size_t channel = 0; channel < channels; ++channel) {
                if (!std::isfinite(value[channel])) throw FormatError("Nonfinite multiband equalizer output");
                stereo[frame * channels + channel] = value[channel];
            }
        }
    }
}
}
