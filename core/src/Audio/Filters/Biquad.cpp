#include "OpenFmod/Audio/Filters/Biquad.hpp"
#include "OpenFmod/Binary/Reader.hpp"
#include <algorithm>
#include <cmath>
namespace OpenFmod::Audio {
BiquadCoefficients DesignPassFilter(float sampleRate, float frequency, float quality, bool highPass) {
    if (!std::isfinite(sampleRate) || sampleRate <= 0 || !std::isfinite(frequency)
        || !std::isfinite(quality) || quality <= 0) throw FormatError("Invalid pass filter settings");
    frequency = std::min(std::clamp(frequency, 10.0f, 22000.0f), sampleRate * 0.49f);
    const float angle = frequency * 6.2831853071795864769f / sampleRate;
    return DesignPassFilterAtAngle(angle, quality, highPass);
}
BiquadCoefficients DesignPassFilterAtAngle(float angle, float quality, bool highPass) {
    if (!std::isfinite(angle) || angle <= 0 || angle >= 3.14159265358979323846f
        || !std::isfinite(quality) || quality <= 0) throw FormatError("Invalid normalized pass filter settings");
    const float cosine = std::cos(angle);
    const float alpha = std::sin(angle) * (0.5f / quality);
    const float scale = 1 / (1 + alpha);
    const float middle = highPass ? -(1 + cosine) * scale : (1 - cosine) * scale;
    const float edge = middle * (highPass ? -0.5f : 0.5f);
    return {edge, middle, edge, -2 * cosine * scale, (1 - alpha) * scale};
}
void Biquad::Configure(const BiquadCoefficients& values, std::uint32_t rampFrames) {
    for (float value : values) {
        if (!std::isfinite(value)) throw FormatError("Nonfinite filter coefficient");
    }
    target = values;
    remaining = rampFrames;
    if (!remaining) coefficients = target;
    else {
        for (std::size_t index = 0; index < step.size(); ++index) {
            step[index] = (target[index] - coefficients[index]) / remaining;
        }
    }
}
void Biquad::Reset() { memory = {}; }
std::array<float, 2> Biquad::Process(std::array<float, 2> input) {
    Process(input.data(), input.size());
    return input;
}
void Biquad::Process(float* input, std::size_t channels) {
    if (!input || !channels || channels > memory.size()) throw FormatError("Invalid filter channel buffer");
    if (remaining) {
        --remaining;
        if (!remaining) coefficients = target;
        else {
            for (std::size_t index = 0; index < step.size(); ++index) coefficients[index] += step[index];
        }
    }
    for (std::size_t channel = 0; channel < channels; ++channel) {
        auto& state = memory[channel];
        const float current = input[channel] - state[0] * coefficients[3] - state[1] * coefficients[4];
        input[channel] = current * coefficients[0] + state[0] * coefficients[1] + state[1] * coefficients[2];
        state[1] = state[0];
        state[0] = current;
    }
}
}
