#ifndef OPENFMOD_AUDIO_FILTERS_BIQUAD_HPP
#define OPENFMOD_AUDIO_FILTERS_BIQUAD_HPP
#include <array>
#include <cstdint>
namespace OpenFmod::Audio {
using BiquadCoefficients = std::array<float, 5>;
BiquadCoefficients DesignPassFilter(float sampleRate, float frequency, float quality, bool highPass);
BiquadCoefficients DesignPassFilterAtAngle(float angle, float quality, bool highPass);
class Biquad {
public:
    void Configure(const BiquadCoefficients& coefficients, std::uint32_t rampFrames = 0);
    void Reset();
    void Process(float* input, std::size_t channels);
    std::array<float, 2> Process(std::array<float, 2> input);
private:
    BiquadCoefficients coefficients{1, 0, 0, 0, 0};
    BiquadCoefficients target{1, 0, 0, 0, 0};
    BiquadCoefficients step{};
    std::array<std::array<float, 2>, 8> memory{};
    std::uint32_t remaining{};
};
}
#endif
