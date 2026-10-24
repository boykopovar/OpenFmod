#ifndef OPENFMOD_STUDIO_EFFECT_PROCESSOR_HPP
#define OPENFMOD_STUDIO_EFFECT_PROCESSOR_HPP
#include "OpenFmod/Studio/Effect/Configuration.hpp"
namespace OpenFmod::Studio {
class EffectProcessor final : public Audio::IAudioProcessor {
public:
    EffectProcessor(const Effect& effect, std::uint32_t sampleRate);
    void SetParameter(std::size_t index, EffectParameter value, std::uint32_t rampFrames = 0);
    void SetBypassed(bool value);
    void Process(float* stereo, std::size_t frames) override;
    void ProcessWithSidechain(float* stereo, std::size_t frames, const float* sidechain, std::size_t channels);
private:
    using Unit = std::variant<Audio::Compressor, Audio::Reverb, Audio::ThreeBandEq, Audio::MultibandEq>;
    static Unit create(const Effect& effect, std::uint32_t sampleRate);
    std::vector<EffectParameter> parameters;
    Unit unit;
    bool bypassed{};
    bool sidechainEnabled{};
};
}
#endif
