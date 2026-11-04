#include "OpenFmod/Audio/Signal/Precision.hpp"
#include "OpenFmod/Audio/Reverb/Reverb.hpp"
#include <cmath>
#include <limits>
namespace OpenFmod::Audio {
namespace {
std::uint32_t requireRate(std::uint32_t rate) {
    if (rate < 8000 || rate > 384000) throw FormatError("Invalid reverb sample rate");
    return rate;
}
float blend(float from, float to, float fraction) { return from + (to - from) * fraction; }
float tap(const DelayLine& line, std::size_t from, std::size_t to, float fraction) {
    const auto value = line.Read(to - 1);
    return from == to ? value : blend(line.Read(from - 1), value, fraction);
}
}
Reverb::Reverb(std::uint32_t rate, ReverbSettings settings)
    : rate(requireRate(rate)), settings(settings), predelay(this->rate / 2 + 1), earlySpread(this->rate / 10 + 1) {
    settings.Validate();
    configure();
    for (const auto delay : parameters.diffuserDelays) diffusers.emplace_back(delay);
    float maximum = 0.061f;
    for (std::size_t index = 0; index < 8; ++index) {
        feedback.emplace_back(static_cast<std::size_t>(maximum * rate + 1));
        maximum *= 1.3200001f;
    }
    previous = parameters;
}
void Reverb::Configure(ReverbSettings value) {
    value.Validate();
    settings = value;
    configure();
}
void Reverb::Reset() {
    predelay.Reset();
    earlySpread.Reset();
    for (auto& line : diffusers) line.Reset();
    for (auto& line : feedback) line.Reset();
    filtered.fill(0);
    lowpassState.fill(0);
    shelf.Reset();
    previous = parameters;
}
float Reverb::filterInput(float input, float fraction) {
    const auto lowpass = blend(previous.lowpass, parameters.lowpass, fraction);
    for (auto& state : lowpassState) {
        state = FlushSubnormal(input * lowpass + (1 - lowpass) * state);
        input = state;
    }
    if (settings.LowShelfDecibels != 0) input = shelf.Process({input, input})[0];
    return input;
}
std::array<float, 8> Reverb::reverberate(float input, float fraction, std::size_t channels) {
    const auto earlyGain = blend(previous.earlyGain, parameters.earlyGain, fraction);
    const auto lateGain = blend(previous.lateGain, parameters.lateGain, fraction);
    const auto normalization = blend(previous.normalization, parameters.normalization, fraction);
    const auto diffusion = blend(previous.diffusion, parameters.diffusion, fraction);
    predelay.Advance();
    predelay.Write(filterInput(input, fraction));
    auto early = tap(predelay, previous.inputDelays[0], parameters.inputDelays[0], fraction) * earlyGain;
    for (std::size_t index = 0; index < diffusers.size(); ++index) {
        auto& line = diffusers[index];
        line.Advance();
        const auto output = line.Read(parameters.diffuserDelays[index] - 1) - 0.63f * early;
        line.Write(early + 0.63f * output);
        early = output;
    }
    earlySpread.Advance();
    earlySpread.Write(early);
    std::array<float, 8> mixed{};
    for (std::size_t index = 0; index < feedback.size(); ++index) {
        auto& line = feedback[index];
        line.Advance();
        const auto returned = blend(line.Read(previous.feedbackDelays[index] - 1) * previous.feedbackGains[index],
            line.Read(parameters.feedbackDelays[index] - 1) * parameters.feedbackGains[index], fraction);
        const auto value = (returned + tap(predelay, previous.inputDelays[index + 1], parameters.inputDelays[index + 1], fraction) * lateGain) * normalization;
        const auto damping = blend(previous.damping[index], parameters.damping[index], fraction);
        filtered[index] = FlushSubnormal(value * damping + (1 - damping) * filtered[index]);
        mixed[index] = filtered[index];
    }
    for (std::size_t stride = 1; stride < mixed.size(); stride *= 2) {
        for (std::size_t block = 0; block < mixed.size(); block += stride * 2) {
            for (std::size_t index = block; index < block + stride; ++index) {
                const auto first = mixed[index];
                const auto second = mixed[index + stride];
                mixed[index] = first + diffusion * second;
                mixed[index + stride] = second - diffusion * first;
            }
        }
    }
    for (std::size_t index = 0; index < feedback.size(); ++index) feedback[index].Write(mixed[(index + 1) % 8]);
    std::array<float, 8> output{};
    for (std::size_t channel = 0; channel < channels; ++channel) {
        float earlyOutput = 0;
        float lateOutput = 0;
        for (std::size_t tapIndex = 0; tapIndex < 8 / channels; ++tapIndex) {
            const auto index = channel + tapIndex * channels;
            earlyOutput += index ? earlySpread.Read(parameters.earlyDelays[index - 1] - 1) : early;
            lateOutput += feedback[index].Read(parameters.outputDelays[index] - 1);
        }
        output[channel] = earlyOutput + lateOutput;
    }
    return output;
}
void Reverb::Process(float* stereo, std::size_t frames) { Process(stereo, frames, 2); }
void Reverb::Process(float* samples, std::size_t frames, std::size_t channels) {
    if (!channels || channels > 8 || (frames && !samples) || frames > std::numeric_limits<std::size_t>::max() / channels) {
        throw FormatError("Invalid reverb buffer");
    }
    for (std::size_t frame = 0; frame < frames; ++frame) {
        float input = 0;
        for (std::size_t channel = 0; channel < channels; ++channel) {
            const auto value = samples[frame * channels + channel];
            if (!std::isfinite(value)) throw FormatError("Nonfinite reverb input");
            input += value;
        }
        const auto wet = reverberate(input, static_cast<float>(frame) / frames, channels);
        for (std::size_t channel = 0; channel < channels; ++channel) {
            auto& value = samples[frame * channels + channel];
            value = value * parameters.dryGain + wet[channel];
            if (!std::isfinite(value)) throw FormatError("Nonfinite reverb output");
        }
    }
    if (frames) previous = parameters;
}
}
