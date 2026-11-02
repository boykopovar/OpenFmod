#ifndef OPENFMOD_AUDIO_DYNAMICS_SIGNALTAP_HPP
#define OPENFMOD_AUDIO_DYNAMICS_SIGNALTAP_HPP
#include "OpenFmod/Audio/Processing/IAudioProcessor.hpp"
#include <vector>
namespace OpenFmod::Audio {
class SignalTap final : public IAudioProcessor {
public:
    void SetGain(float value);
    void Process(float* stereo, std::size_t frames) override;
    const std::vector<float>& GetSamples(std::size_t frames) const;
private:
    float gain{1};
    std::vector<float> samples;
    bool initialized{};
};
}
#endif
