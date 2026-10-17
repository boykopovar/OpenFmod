#ifndef OPENFMOD_AUDIO_DYNAMICS_COMPRESSOR_HPP
#define OPENFMOD_AUDIO_DYNAMICS_COMPRESSOR_HPP
#include "OpenFmod/Audio/IAudioProcessor.hpp"
#include <array>
#include <cstdint>
namespace OpenFmod::Audio {
struct CompressorSettings {
    float ThresholdDecibels{};
    float Ratio{2.5f};
    float AttackMilliseconds{20};
    float ReleaseMilliseconds{200};
    float MakeupDecibels{};
    bool Linked{true};
};
class Compressor final : public IAudioProcessor {
public:
    explicit Compressor(std::uint32_t sampleRate, CompressorSettings settings = {});
    void Configure(CompressorSettings settings);
    void Reset();
    void Process(float* stereo, std::size_t frames) override;
    void ProcessWithSidechain(float* stereo, std::size_t frames, const float* sidechain, std::size_t channels);
private:
    float gain(float energy, std::size_t channel);
    std::uint32_t sampleRate;
    CompressorSettings settings;
    float threshold{};
    float exponent{};
    float attack{};
    float release{};
    float makeup{};
    std::array<float, 2> first{};
    std::array<float, 2> second{};
};
}
#endif
