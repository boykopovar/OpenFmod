#include "OpenFmod/Audio/Reverb/Reverb.hpp"
#include <cmath>
namespace OpenFmod::Audio {
namespace {
float dampingCoefficient(float magnitude, float cosine) {
    if (magnitude == 1) return 1;
    const auto squared = magnitude * magnitude;
    const auto difference = squared - 1;
    const auto middle = 2 * (1 - cosine * squared);
    const auto root = std::sqrt(std::max(0.0f, middle * middle - 4 * difference * difference));
    return 1 - std::clamp((root - middle) * (0.5f / difference), 0.0f, 0.99f);
}
}
void Reverb::configure() {
    const auto frames = [this](float seconds) { return std::max<std::size_t>(1, static_cast<std::size_t>(seconds * rate)); };
    parameters.inputDelays[0] = frames(settings.EarlyDelayMilliseconds * 0.001f);
    const auto late = (settings.EarlyDelayMilliseconds + settings.LateDelayMilliseconds) * 0.001f;
    parameters.inputDelays[1] = frames(late);
    float offset = 0.0187f;
    float spread = 0.005f;
    for (std::size_t index = 0; index < 7; ++index) {
        parameters.inputDelays[index + 2] = frames(late + offset);
        parameters.earlyDelays[index] = frames(spread);
        offset *= 1.29f;
        spread *= 1.59f;
    }
    parameters.diffuserDelays = {frames(0.0059f), frames(0.0077f)};
    const auto density = settings.DensityPercent;
    float delay = std::clamp(density * density * (density * 0.0000009f) + 0.1f, 0.1f, 1.0f) * 0.061f;
    float outputDelay = rate * 0.0015f;
    float gainEnergy = 0;
    const auto cosine = std::cos(settings.HighFrequencyReference * 6.2831854820251465f / rate);
    for (std::size_t index = 0; index < 8; ++index) {
        parameters.feedbackDelays[index] = static_cast<std::size_t>(delay * rate + 0.5f);
        parameters.outputDelays[index] = static_cast<std::size_t>(outputDelay + 0.5f);
        const auto attenuation = -59999.99609375f / settings.DecayMilliseconds * delay;
        parameters.feedbackGains[index] = std::pow(10.0f, attenuation * 0.05f);
        const auto highAttenuation = -5999999.5f / (settings.HighFrequencyDecayPercent * settings.DecayMilliseconds) * delay;
        parameters.damping[index] = dampingCoefficient(std::min(1.0f, std::pow(10.0f, (highAttenuation - attenuation) * 0.05f)), cosine);
        gainEnergy += parameters.feedbackGains[index] * parameters.feedbackGains[index];
        delay *= 1.3200001f;
        outputDelay *= 1.47f;
    }
    const auto angle = settings.DiffusionPercent * 0.007853982038795948f;
    parameters.diffusion = std::tan(angle);
    parameters.normalization = std::pow(std::cos(angle), 3.0f);
    parameters.lowpass = dampingCoefficient(0.8413951416f, std::cos(settings.HighCutFrequency * 6.2831854820251465f / rate));
    const auto tangent = std::tan(settings.LowShelfFrequency * 3.1415927410125732f / rate);
    const auto scaled = tangent / std::exp(settings.LowShelfDecibels * 0.057564627379179f);
    const auto inverse = 1 / ((scaled + 1.414213538f) * scaled + 1);
    shelf.Configure({((tangent + 1.414213538f) * tangent + 1) * inverse,
        2 * (tangent * tangent - 1) * inverse, ((tangent - 1.414213538f) * tangent + 1) * inverse,
        2 * (scaled * scaled - 1) * inverse, ((scaled - 1.414213538f) * scaled + 1) * inverse}, 0);
    const auto wet = settings.WetDecibels <= -80 ? 0 : std::pow(10.0f, settings.WetDecibels * 0.05f);
    const auto mix = settings.LateMixPercent * 0.01f;
    const auto mixGain = std::sqrt(2 / (mix * mix + (1 - mix) * (1 - mix))) * wet * 0.35355338f;
    parameters.earlyGain = mixGain * (1 - mix);
    parameters.lateGain = mixGain * mix * std::sqrt(1 - gainEnergy / 8);
    parameters.dryGain = settings.DryDecibels <= -80 ? 0 : std::pow(10.0f, settings.DryDecibels * 0.05f);
}
}
