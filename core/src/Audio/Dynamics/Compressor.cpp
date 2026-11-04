#include "OpenFmod/Audio/Signal/Precision.hpp"
#include "OpenFmod/Audio/Dynamics/Compressor.hpp"
#include "OpenFmod/Binary/Reader.hpp"
#include <cmath>
#include <limits>
namespace OpenFmod::Audio {
Compressor::Compressor(std::uint32_t sampleRate, CompressorSettings settings) : sampleRate(sampleRate) {
    if (!sampleRate) throw FormatError("Invalid compressor sample rate");
    Configure(settings);
}
void Compressor::Configure(CompressorSettings value) {
    if (!std::isfinite(value.ThresholdDecibels) || value.ThresholdDecibels < -60 || value.ThresholdDecibels > 0
        || !std::isfinite(value.Ratio) || value.Ratio < 1 || value.Ratio > 50
        || !std::isfinite(value.AttackMilliseconds) || value.AttackMilliseconds < 0.1f || value.AttackMilliseconds > 500
        || !std::isfinite(value.ReleaseMilliseconds) || value.ReleaseMilliseconds < 10 || value.ReleaseMilliseconds > 5000
        || !std::isfinite(value.MakeupDecibels) || value.MakeupDecibels < -30 || value.MakeupDecibels > 30) {
        throw FormatError("Invalid compressor settings");
    }
    settings = value;
    threshold = std::pow(10.0f, value.ThresholdDecibels * 0.1f);
    exponent = 0.5f / value.Ratio - 0.5f;
    attack = 1 - std::exp(-3.111269950866699f / (value.AttackMilliseconds * 0.001f * sampleRate));
    release = 1 - std::exp(-3.111269950866699f / (value.ReleaseMilliseconds * 0.001f * sampleRate));
    makeup = std::pow(10.0f, value.MakeupDecibels * 0.05f);
}
void Compressor::Reset() { first = {}; second = {}; }
float Compressor::gain(float energy, std::size_t channel) {
    const float coefficient = energy > second[channel] ? attack : release;
    first[channel] = FlushSubnormal(first[channel] + (energy - first[channel]) * coefficient);
    second[channel] = FlushSubnormal(second[channel] + (first[channel] - second[channel]) * coefficient);
    return second[channel] > threshold ? makeup * std::pow(second[channel] / threshold, exponent) : makeup;
}
void Compressor::Process(float* stereo, std::size_t frames) {
    Process(stereo, frames, 2);
}
void Compressor::Process(float* samples, std::size_t frames, std::size_t channels) {
    ProcessWithSidechain(samples, frames, channels, samples, channels);
}
void Compressor::ProcessWithSidechain(float* stereo, std::size_t frames, const float* sidechain, std::size_t channels) {
    ProcessWithSidechain(stereo, frames, 2, sidechain, channels);
}
void Compressor::ProcessWithSidechain(float* stereo, std::size_t frames, std::size_t outputChannels, const float* sidechain, std::size_t channels) {
    if (!outputChannels || outputChannels > 8 || !channels || channels > 32 || frames > std::numeric_limits<std::size_t>::max() / channels
        || frames > std::numeric_limits<std::size_t>::max() / outputChannels || (frames && (!stereo || !sidechain))) {
        throw FormatError("Invalid compressor audio buffers");
    }
    const bool linked = settings.Linked || channels != outputChannels;
    for (std::size_t frame = 0; frame < frames; ++frame) {
        std::array<float, 8> gains{};
        if (linked) {
            float energy = 0;
            for (std::size_t channel = 0; channel < channels; ++channel) {
                const float value = sidechain[frame * channels + channel];
                energy += value * value;
            }
            gains.fill(gain(energy, 0));
        } else {
            for (std::size_t channel = 0; channel < outputChannels; ++channel) {
                const float value = sidechain[frame * channels + channel];
                gains[channel] = gain(value * value, channel);
            }
        }
        for (std::size_t channel = 0; channel < outputChannels; ++channel) {
            const float value = stereo[frame * outputChannels + channel] * gains[channel];
            if (!std::isfinite(value) || !std::isfinite(second[channel])) throw FormatError("Nonfinite compressor signal");
            stereo[frame * outputChannels + channel] = value;
        }
    }
}
}
