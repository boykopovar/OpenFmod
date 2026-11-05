#include "OpenFmod/Studio/Playback/Sound.hpp"
#include "OpenFmod/Audio/SilenceSource.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
namespace OpenFmod::Studio {
SoundPlayback::SoundPlayback(const Library& library, SourceLibrary& sources, Audio::Mixer& mixer, const Bank::Guid& instrument, const AutomationBindings& bindings, const std::function<float()>& random, std::uint32_t sampleRate, ProgrammerSoundFactory programmer)
    : library(library), sources(sources), mixer(mixer), model(library.Retain<Instrument>(instrument)),
      properties(library, *model, bindings, random), sampleRate(sampleRate), programmer(std::move(programmer)) {
    if (!sampleRate) throw FormatError("Sound playback sample rate is zero");
    if (!std::holds_alternative<WaveInstrument>(model->Definition.Source)
        && !std::holds_alternative<SilenceInstrument>(model->Definition.Source)
        && !std::holds_alternative<ProgrammerInstrument>(model->Definition.Source)) throw FormatError("Sound playback requires a wave, silence or programmer instrument");
}
void SoundPlayback::Start(const SoundStart& start, const AutomationInput& input) {
    if (started) throw FormatError("Sound playback has already started");
    properties.Update(input, 0);
    std::unique_ptr<Audio::IAudioSource> source;
    Audio::VoiceSettings settings;
    if (const auto* wave = std::get_if<WaveInstrument>(&model->Definition.Source)) {
        settings = SoundPosition(library.Require<Sound::Sample>(wave->Source), start);
        source = sources.Create(wave->Source);
    } else if (const auto* request = std::get_if<ProgrammerInstrument>(&model->Definition.Source)) {
        if (!programmer) throw FormatError("Programmer instrument requires a sound provider: " + request->Name);
        source = programmer(request->Name);
        if (!source) throw FormatError("Programmer sound provider returned an empty source");
        const auto format = source->GetFormat();
        if (format.Frames > std::numeric_limits<std::uint32_t>::max()) throw FormatError("Programmer sound duration overflow");
        Sound::Sample sample;
        sample.Rate = format.Rate;
        sample.Frames = static_cast<std::uint32_t>(format.Frames);
        settings = SoundPosition(sample, start);
    } else {
        const auto duration = properties.Get(29);
        const auto frames = duration < 0 ? 1 : static_cast<double>(duration) * sampleRate / 1000;
        if (frames > std::numeric_limits<std::uint32_t>::max()) throw FormatError("Silence duration overflow");
        Sound::Sample sample;
        sample.Rate = sampleRate;
        sample.Frames = static_cast<std::uint32_t>(frames);
        if (sample.Frames) settings = SoundPosition(sample, start);
        if (duration < 0) settings.Repeats = -1;
        source = std::make_unique<Audio::SilenceSource>(Audio::Format{sampleRate, 1, sample.Frames});
    }
    if (!std::holds_alternative<SilenceInstrument>(model->Definition.Source)) {
        settings.GainRampFrames = 64;
        settings.FadeInFrames = 64;
    }
    settings.Gain = properties.GetGain();
    settings.Pitch = properties.GetPitch(input);
    voice = mixer.Play(std::move(source), settings);
    gain = settings.Gain;
    pitch = settings.Pitch;
    started = true;
}
void SoundPlayback::Update(const AutomationInput& input, double milliseconds, float parentGain, float parentPitch) {
    if (!std::isfinite(milliseconds) || milliseconds < 0 || !std::isfinite(parentGain) || parentGain < 0
        || !std::isfinite(parentPitch) || parentPitch < 0) throw FormatError("Invalid sound playback update");
    if (!started) throw FormatError("Sound playback has not started");
    if (!IsPlaying()) return;
    properties.Update(input, milliseconds);
    if (releasing && properties.IsFinished(milliseconds)) {
        mixer.Stop(*voice);
        return;
    }
    const auto nextGain = parentGain * properties.GetGain();
    const auto nextPitch = parentPitch * properties.GetPitch(input);
    if (nextGain != gain) { mixer.SetVoiceGain(*voice, nextGain); gain = nextGain; }
    if (nextPitch != pitch) { mixer.SetVoicePitch(*voice, nextPitch); pitch = nextPitch; }
}
void SoundPlayback::Stop(bool immediate, double milliseconds) {
    if (!started) throw FormatError("Sound playback has not started");
    if (!IsPlaying()) return;
    if (immediate) mixer.Stop(*voice);
    else if (!releasing) {
        properties.Release(milliseconds);
        releasing = true;
        if (properties.IsFinished(milliseconds)) mixer.Stop(*voice);
    }
}
bool SoundPlayback::IsPlaying() const { return voice && mixer.IsPlaying(*voice); }
void SoundPlayback::End(bool cutOff, double milliseconds) {
    if (!started || !std::isfinite(milliseconds) || milliseconds < 0) throw FormatError("Invalid sound end time");
    if (!IsPlaying()) return;
    if (cutOff || (model->Settings.Flags & 1) || properties.HasEnvelope()) Stop(false, milliseconds);
    else mixer.Finish(*voice);
}
SoundPlayback::~SoundPlayback() { if (voice) mixer.Stop(*voice); }
}
