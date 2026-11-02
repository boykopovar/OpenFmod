#ifndef OPENFMOD_AUDIO_FILTERS_THREEBANDEQ_HPP
#define OPENFMOD_AUDIO_FILTERS_THREEBANDEQ_HPP
#include "OpenFmod/Audio/Filters/Biquad.hpp"
#include "OpenFmod/Audio/Processing/IAudioProcessor.hpp"
namespace OpenFmod::Audio {
struct ThreeBandSettings {
    std::array<float, 3> GainDecibels{};
    float LowCrossover{400};
    float HighCrossover{4000};
    std::uint32_t Slope{1};
};
class ThreeBandEq final : public IAudioProcessor {
public:
    explicit ThreeBandEq(std::uint32_t sampleRate, ThreeBandSettings settings = {});
    void Configure(ThreeBandSettings settings);
    void Process(float* stereo, std::size_t frames) override;
private:
    void configureFilters(std::uint32_t rampFrames);
    std::array<float, 2> filter(std::size_t branch, std::array<float, 2> input);
    std::uint32_t sampleRate;
    ThreeBandSettings settings;
    std::array<std::array<Biquad, 4>, 4> branches;
    std::array<float, 3> gains{};
    std::array<float, 3> targets{};
    std::array<float, 3> increments{};
    std::array<std::uint32_t, 3> remaining{};
    bool initialized{};
};
}
#endif
