#ifndef OPENFMOD_AUDIO_FILTERS_MULTIBANDEQ_HPP
#define OPENFMOD_AUDIO_FILTERS_MULTIBANDEQ_HPP
#include "OpenFmod/Audio/Filters/EqualizerDesign.hpp"
#include "OpenFmod/Audio/IAudioProcessor.hpp"
namespace OpenFmod::Audio {
class MultibandEq final : public IAudioProcessor {
public:
    explicit MultibandEq(std::uint32_t sampleRate);
    void Configure(std::size_t band, EqualizerBand settings, std::uint32_t rampFrames = 0);
    void Process(float* stereo, std::size_t frames) override;
private:
    struct Band {
        EqualizerBand Settings;
        std::array<Biquad, 4> Filters;
        std::size_t Stages{};
    };
    std::uint32_t sampleRate;
    std::array<Band, 5> bands;
};
}
#endif
