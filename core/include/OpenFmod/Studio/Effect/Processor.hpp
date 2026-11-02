#ifndef OPENFMOD_STUDIO_EFFECT_PROCESSOR_HPP
#define OPENFMOD_STUDIO_EFFECT_PROCESSOR_HPP
#include "OpenFmod/Studio/Effect/Configuration.hpp"
#include "OpenFmod/Audio/Dynamics/SignalTap.hpp"
#include <memory>
#include "OpenFmod/Audio/Processing/IAudioTransform.hpp"
namespace OpenFmod::Studio {
class EffectProcessor final : public Audio::IAudioProcessor, public Audio::IAudioTransform {
public:
    EffectProcessor(const Effect& effect, std::uint32_t sampleRate);
    void SetParameter(std::size_t index, EffectParameter value, std::uint32_t rampFrames = 0);
    void SetBypassed(bool value);
    void SetSidechains(std::vector<std::shared_ptr<Audio::SignalTap>> sources);
    void Process(float* stereo, std::size_t frames) override;
    Audio::SpeakerMode GetOutputLayout(Audio::SpeakerMode input) const override;
    void Process(const float* input, float* output, std::size_t frames, Audio::SpeakerMode inputLayout) override;
    void ProcessWithSidechain(float* stereo, std::size_t frames, const float* sidechain, std::size_t channels);
private:
    using Unit = std::variant<Audio::Compressor, Audio::Reverb, Audio::Panner, Audio::ThreeBandEq, Audio::MultibandEq>;
    void prepareSidechain(std::size_t frames);
    static Unit create(const Effect& effect, std::uint32_t sampleRate);
    std::vector<EffectParameter> parameters;
    Unit unit;
    bool bypassed{};
    bool sidechainEnabled{};
    std::vector<std::shared_ptr<Audio::SignalTap>> sidechains;
    std::vector<float> mixedSidechain;
};
}
#endif
