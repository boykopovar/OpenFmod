#ifndef OPENFMOD_STUDIO_PLAYBACK_SOUND_HPP
#define OPENFMOD_STUDIO_PLAYBACK_SOUND_HPP
#include "OpenFmod/Studio/Project/Sources.hpp"
#include "OpenFmod/Studio/Playback/Controls.hpp"
#include "OpenFmod/Studio/Playback/IPlayback.hpp"
#include "OpenFmod/Audio/Mixer.hpp"
namespace OpenFmod::Studio {
using ProgrammerSoundFactory = std::function<std::unique_ptr<Audio::IAudioSource>(const std::string&)>;
Audio::VoiceSettings SoundPosition(const Sound::Sample& sample, const SoundStart& start);
class SoundPlayback final : public IPlayback {
public:
    SoundPlayback(const Library& library, SourceLibrary& sources, Audio::Mixer& mixer, const Bank::Guid& instrument, const AutomationBindings& bindings, const std::function<float()>& random, std::uint32_t sampleRate, ProgrammerSoundFactory programmer = {});
    ~SoundPlayback() override;
    SoundPlayback(const SoundPlayback&) = delete;
    SoundPlayback& operator=(const SoundPlayback&) = delete;
    void Start(const SoundStart& start, const AutomationInput& input) override;
    void Update(const AutomationInput& input, double milliseconds, float parentGain = 1, float parentPitch = 1) override;
    void Stop(bool immediate, double milliseconds) override;
    void End(bool cutOff, double milliseconds) override;
    bool IsPlaying() const override;
private:
    const Library& library;
    SourceLibrary& sources;
    Audio::Mixer& mixer;
    std::shared_ptr<const Instrument> model;
    InstrumentControls properties;
    std::uint32_t sampleRate;
    ProgrammerSoundFactory programmer;
    std::optional<std::uint64_t> voice;
    float gain{};
    float pitch{};
    bool started{};
    bool releasing{};
};
}
#endif
