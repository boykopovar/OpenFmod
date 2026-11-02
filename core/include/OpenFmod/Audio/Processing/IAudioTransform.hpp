#ifndef OPENFMOD_AUDIO_IAUDIOTRANSFORM_HPP
#define OPENFMOD_AUDIO_IAUDIOTRANSFORM_HPP
#include "OpenFmod/Audio/Spatial/Speakers.hpp"
namespace OpenFmod::Audio {
class IAudioTransform {
public:
    virtual ~IAudioTransform() = default;
    virtual SpeakerMode GetOutputLayout(SpeakerMode input) const = 0;
    virtual void Process(const float* input, float* output, std::size_t frames, SpeakerMode inputLayout) = 0;
};
}
#endif
