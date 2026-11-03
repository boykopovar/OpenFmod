#include "OpenFmod/Audio/Spatial/MixMatrix.hpp"
#include "OpenFmod/Binary/Reader.hpp"
#include <cmath>
#include <limits>
namespace OpenFmod::Audio {
SpeakerMode SpeakerLayout(std::size_t channels) {
    switch (channels) {
        case 1: return SpeakerMode::Mono;
        case 2: return SpeakerMode::Stereo;
        case 4: return SpeakerMode::Quad;
        case 5: return SpeakerMode::Surround;
        case 6: return SpeakerMode::Surround51;
        case 8: return SpeakerMode::Surround71;
        default: throw FormatError("Unsupported channel count");
    }
}
MixMatrix::MixMatrix(SpeakerMode input, SpeakerMode output)
    : identity(input == output), inputChannels(SpeakerChannels(input)), outputChannels(SpeakerChannels(output)), coefficients(identity ? 0 : inputChannels * outputChannels) {
    if (identity) return;
    if (output == SpeakerMode::Stereo) coefficients = Speakers(input).StereoDownmix();
    else if (input == SpeakerMode::Mono) {
        const bool lowFrequency = outputChannels >= 6;
        const auto gain = 1 / std::sqrt(static_cast<float>(outputChannels - lowFrequency));
        for (std::size_t channel = 0; channel < outputChannels; ++channel) coefficients[channel] = lowFrequency && channel == 3 ? 0 : gain;
    } else if (output == SpeakerMode::Mono) {
        const bool lowFrequency = inputChannels >= 6;
        const auto gain = 1 / std::sqrt(static_cast<float>(inputChannels - lowFrequency));
        for (std::size_t channel = 0; channel < inputChannels; ++channel) coefficients[channel] = lowFrequency && channel == 3 ? 1 : gain;
    } else if (input == SpeakerMode::Stereo) {
        coefficients[0] = 1;
        coefficients[inputChannels + 1] = 1;
    } else throw FormatError("Unsupported conversion between surround layouts");
}
std::size_t MixMatrix::GetInputChannels() const { return inputChannels; }
std::size_t MixMatrix::GetOutputChannels() const { return outputChannels; }
void MixMatrix::Add(const float* input, float* output, std::size_t frames, float gain) const {
    if ((frames && (!input || !output || input == output)) || !std::isfinite(gain)
        || frames > std::numeric_limits<std::size_t>::max() / inputChannels
        || frames > std::numeric_limits<std::size_t>::max() / outputChannels) throw FormatError("Invalid channel conversion buffer");
    if (identity) {
        for (std::size_t index = 0; index < frames * inputChannels; ++index) output[index] += input[index] * gain;
        return;
    }
    for (std::size_t frame = 0; frame < frames; ++frame) {
        for (std::size_t destination = 0; destination < outputChannels; ++destination) {
            float value = 0;
            for (std::size_t source = 0; source < inputChannels; ++source) value += input[frame * inputChannels + source] * coefficients[destination * inputChannels + source];
            output[frame * outputChannels + destination] += value * gain;
        }
    }
}
}
