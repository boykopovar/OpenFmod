#ifndef OPENFMOD_AUDIO_DYNAMICS_SIGNALTAP_HPP
#define OPENFMOD_AUDIO_DYNAMICS_SIGNALTAP_HPP
#include "OpenFmod/Audio/Processing/IAudioProcessor.hpp"
#include <vector>
#include "OpenFmod/Audio/Processing/IAudioTransform.hpp"
namespace OpenFmod::Audio {
class SignalTap final : public IAudioProcessor, public IAudioTransform {
public:
    void SetGain(float value);
    void Process(float* stereo, std::size_t frames) override;
    SpeakerMode GetOutputLayout(SpeakerMode input) const override;
    void Process(const float* input, float* output, std::size_t frames, SpeakerMode inputLayout) override;
    const std::vector<float>& GetSamples(std::size_t frames) const;
private:
    float gain{1};
    std::vector<float> samples;
    bool initialized{};
};
}
#endif
