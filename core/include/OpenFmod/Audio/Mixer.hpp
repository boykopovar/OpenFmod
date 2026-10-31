#ifndef OPENFMOD_AUDIO_MIXER_HPP
#define OPENFMOD_AUDIO_MIXER_HPP
#include "OpenFmod/Audio/Voice.hpp"
#include <map>
#include <mutex>
namespace OpenFmod::Audio {
class Mixer {
public:
    explicit Mixer(std::uint32_t rate);
    std::uint64_t Play(std::unique_ptr<IAudioSource> source, VoiceSettings settings = {});
    bool Stop(std::uint64_t voice);
    void Finish(std::uint64_t voice);
    bool IsPlaying(std::uint64_t voice) const;
    void SetGain(float value);
    void SetPitch(float value);
    void SetVoiceGain(std::uint64_t voice, float value);
    void SetVoicePitch(std::uint64_t voice, float value);
    std::optional<std::uint64_t> FramesUntilNextEnd() const;
    void Render(float* stereo, std::size_t frames);
private:
    Voice& requireVoice(std::uint64_t voice);
    mutable std::mutex mutex;
    std::map<std::uint64_t, Voice> voices;
    std::uint64_t nextId{1};
    std::uint32_t rate;
    float gain{1};
    float pitch{1};
};
}
#endif
