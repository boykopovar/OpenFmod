#ifndef OPENFMOD_AUDIO_IAUDIOPROCESSOR_HPP
#define OPENFMOD_AUDIO_IAUDIOPROCESSOR_HPP
#include <cstddef>
namespace OpenFmod::Audio {
class IAudioProcessor {
public:
    virtual ~IAudioProcessor() = default;
    virtual void Process(float* stereo, std::size_t frames) = 0;
};
}
#endif
