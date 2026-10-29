#ifndef OPENFMOD_STUDIO_PLAYBACK_SOUND_HPP
#define OPENFMOD_STUDIO_PLAYBACK_SOUND_HPP
#include "OpenFmod/Studio/Project/Sources.hpp"
#include "OpenFmod/Studio/Automation/PropertySet.hpp"
#include "OpenFmod/Audio/Mixer.hpp"
namespace OpenFmod::Studio {
struct SoundStart {
    std::uint32_t OffsetTicks{};
    float OffsetPercent{};
    std::int32_t Repeats{};
};
Audio::VoiceSettings SoundPosition(const Sound::Sample& sample, const SoundStart& start);
class SoundPlayback {
public:
    SoundPlayback(const Library& library, SourceLibrary& sources, Audio::Mixer& mixer, const Bank::Guid& instrument, const AutomationBindings& bindings, const std::function<float()>& random, std::uint32_t sampleRate);
    ~SoundPlayback();
    SoundPlayback(const SoundPlayback&) = delete;
    SoundPlayback& operator=(const SoundPlayback&) = delete;
    void Start(const SoundStart& start, const AutomationInput& input);
    void Update(const AutomationInput& input, double milliseconds, float parentGain = 1, float parentPitch = 1);
    void Stop(bool immediate, double milliseconds);
    bool IsPlaying() const;
private:
    static std::map<std::uint32_t, float> baseProperties(const Instrument& model);
    float pitch(const AutomationInput& input) const;
    const Library& library;
    SourceLibrary& sources;
    Audio::Mixer& mixer;
    std::shared_ptr<const Instrument> model;
    PropertySet properties;
    std::uint32_t sampleRate;
    std::optional<std::uint64_t> voice;
    bool started{};
    bool releasing{};
};
}
#endif
