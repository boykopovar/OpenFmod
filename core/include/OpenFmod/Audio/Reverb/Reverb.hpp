#ifndef OPENFMOD_AUDIO_REVERB_REVERB_HPP
#define OPENFMOD_AUDIO_REVERB_REVERB_HPP
#include "OpenFmod/Audio/Delay/Line.hpp"
#include "OpenFmod/Audio/Filters/Biquad.hpp"
#include "OpenFmod/Audio/IAudioProcessor.hpp"
#include "OpenFmod/Audio/Reverb/Settings.hpp"
#include <array>
namespace OpenFmod::Audio {
class Reverb final : public IAudioProcessor {
public:
    Reverb(std::uint32_t rate, ReverbSettings settings = {});
    void Configure(ReverbSettings settings);
    void Reset();
    void Process(float* stereo, std::size_t frames) override;
private:
    void configure();
    float filterInput(float input, float fraction);
    std::array<float, 2> reverberate(float input, float fraction);
    std::uint32_t rate;
    ReverbSettings settings;
    DelayLine predelay;
    DelayLine earlySpread;
    std::vector<DelayLine> diffusers;
    std::vector<DelayLine> feedback;
    struct Parameters {
        std::array<std::size_t, 9> inputDelays{};
        std::array<std::size_t, 7> earlyDelays{};
        std::array<std::size_t, 2> diffuserDelays{};
        std::array<std::size_t, 8> feedbackDelays{};
        std::array<std::size_t, 8> outputDelays{};
        std::array<float, 8> feedbackGains{};
        std::array<float, 8> damping{};
        float lowpass{};
        float diffusion{};
        float normalization{};
        float earlyGain{};
        float lateGain{};
        float dryGain{};
    } parameters, previous;
    std::array<float, 8> filtered{};
    Biquad shelf;
    std::array<float, 2> lowpassState{};
};
}
#endif
