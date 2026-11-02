#ifndef OPENFMOD_AUDIO_SPATIAL_MIXMATRIX_HPP
#define OPENFMOD_AUDIO_SPATIAL_MIXMATRIX_HPP
#include "OpenFmod/Audio/Spatial/Speakers.hpp"
namespace OpenFmod::Audio {
SpeakerMode SpeakerLayout(std::size_t channels);
class MixMatrix {
public:
    MixMatrix(SpeakerMode input, SpeakerMode output);
    std::size_t GetInputChannels() const;
    std::size_t GetOutputChannels() const;
    void Add(const float* input, float* output, std::size_t frames, float gain = 1) const;
private:
    std::size_t inputChannels;
    std::size_t outputChannels;
    std::vector<float> coefficients;
};
}
#endif
