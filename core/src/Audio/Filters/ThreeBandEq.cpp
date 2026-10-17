#include "OpenFmod/Audio/Filters/ThreeBandEq.hpp"
#include "OpenFmod/Binary/Reader.hpp"
#include <cmath>
#include <limits>
namespace OpenFmod::Audio {
ThreeBandEq::ThreeBandEq(std::uint32_t sampleRate, ThreeBandSettings settings) : sampleRate(sampleRate) {
    if (!sampleRate) throw FormatError("Invalid equalizer sample rate");
    Configure(settings);
}
void ThreeBandEq::Configure(ThreeBandSettings values) {
    if (values.Slope > 2 || !std::isfinite(values.LowCrossover) || !std::isfinite(values.HighCrossover)
        || values.LowCrossover < 10 || values.LowCrossover > 22000
        || values.HighCrossover < 10 || values.HighCrossover > 22000) {
        throw FormatError("Invalid three-band crossover settings");
    }
    for (float gain : values.GainDecibels) {
        if (!std::isfinite(gain) || gain < -80 || gain > 10) throw FormatError("Invalid equalizer gain");
    }
    const bool reset = !initialized || settings.Slope != values.Slope;
    const bool changed = reset || settings.LowCrossover != values.LowCrossover || settings.HighCrossover != values.HighCrossover;
    for (std::size_t index = 0; index < gains.size(); ++index) {
        if (initialized && settings.GainDecibels[index] == values.GainDecibels[index]) continue;
        targets[index] = values.GainDecibels[index] <= -80 ? 0 : std::pow(10.0f, values.GainDecibels[index] / 20);
        remaining[index] = initialized ? 256 : 0;
        increments[index] = (targets[index] - gains[index]) / 256;
        if (!initialized) gains[index] = targets[index];
    }
    settings = values;
    if (reset) {
        for (auto& branch : branches) for (auto& stage : branch) stage.Reset();
    }
    if (changed) configureFilters(reset ? 0 : 1024);
    initialized = true;
}
void ThreeBandEq::configureFilters(std::uint32_t rampFrames) {
    const auto stages = std::size_t{1} << settings.Slope;
    for (std::size_t branch = 0; branch < branches.size(); ++branch) {
        const float frequency = branch < 2 ? settings.LowCrossover : settings.HighCrossover;
        for (std::size_t stage = 0; stage < stages; ++stage) {
            const float quality = settings.Slope == 0 ? 0.5f : settings.Slope == 1 ? 0.7071067811865475f
                : stage % 2 ? 1.34f : 0.54f;
            branches[branch][stage].Configure(DesignPassFilter(static_cast<float>(sampleRate), frequency, quality, branch % 2 != 0), rampFrames);
        }
    }
}
std::array<float, 2> ThreeBandEq::filter(std::size_t branch, std::array<float, 2> input) {
    const auto stages = std::size_t{1} << settings.Slope;
    for (std::size_t stage = 0; stage < stages; ++stage) input = branches[branch][stage].Process(input);
    return input;
}
void ThreeBandEq::Process(float* stereo, std::size_t frames) {
    if (frames > std::numeric_limits<std::size_t>::max() / 2 || (frames && !stereo)) {
        throw FormatError("Invalid equalizer buffer");
    }
    for (std::size_t frame = 0; frame < frames; ++frame) {
        for (std::size_t index = 0; index < gains.size(); ++index) {
            if (remaining[index]) gains[index] = --remaining[index] ? gains[index] + increments[index] : targets[index];
        }
        const std::array<float, 2> input{stereo[frame * 2], stereo[frame * 2 + 1]};
        const auto low = filter(0, input);
        const auto mid = filter(2, filter(1, input));
        const auto high = filter(3, input);
        for (std::size_t channel = 0; channel < 2; ++channel) {
            const float value = low[channel] * gains[0] + mid[channel] * gains[1] * (settings.Slope ? 1 : -1) + high[channel] * gains[2];
            if (!std::isfinite(value)) throw FormatError("Nonfinite equalizer output");
            stereo[frame * 2 + channel] = value;
        }
    }
}
}
