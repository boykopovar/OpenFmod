#include "OpenFmod/Studio/Playback/Sound.hpp"
#include "OpenFmod/Studio/Levels.hpp"
#include "OpenFmod/Audio/SilenceSource.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
namespace OpenFmod::Studio {
std::map<std::uint32_t, float> SoundPlayback::baseProperties(const Instrument& model) {
    auto base = model.Settings.Properties;
    base.emplace(4, 1);
    base.emplace(5, 0);
    base.emplace(6, 0);
    if (const auto* silence = std::get_if<SilenceInstrument>(&model.Definition.Source)) base.emplace(29, silence->DurationMilliseconds);
    return base;
}
SoundPlayback::SoundPlayback(const Library& library, SourceLibrary& sources, Audio::Mixer& mixer, const Bank::Guid& instrument, const AutomationBindings& bindings, const std::function<float()>& random, std::uint32_t sampleRate)
    : library(library), sources(sources), mixer(mixer), model(library.Retain<Instrument>(instrument)),
      properties(library, baseProperties(*model), model->Properties, bindings, random), sampleRate(sampleRate) {
    if (!sampleRate) throw FormatError("Sound playback sample rate is zero");
    if (!std::holds_alternative<WaveInstrument>(model->Definition.Source)
        && !std::holds_alternative<SilenceInstrument>(model->Definition.Source)) throw FormatError("Sound playback requires a wave or silence instrument");
}
void SoundPlayback::Start(const SoundStart& start, const AutomationInput& input) {
    if (started) throw FormatError("Sound playback has already started");
    properties.Update(input, 0);
    std::unique_ptr<Audio::IAudioSource> source;
    Audio::VoiceSettings settings;
    if (const auto* wave = std::get_if<WaveInstrument>(&model->Definition.Source)) {
        settings = SoundPosition(library.Require<Sound::Sample>(wave->Source), start);
        source = sources.Create(wave->Source);
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
    settings.Gain = VolumeGain(properties.Get(0)) * properties.Get(4);
    settings.Pitch = pitch(input);
    voice = mixer.Play(std::move(source), settings);
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
    mixer.SetVoiceGain(*voice, parentGain * VolumeGain(properties.Get(0)) * properties.Get(4));
    mixer.SetVoicePitch(*voice, parentPitch * pitch(input));
}
float SoundPlayback::pitch(const AutomationInput& input) const {
    float multiplier = 1;
    const auto& settings = model->Settings;
    if (settings.AutoPitchParameter != Bank::Guid{}) {
        if (!input.Parameters) throw FormatError("Missing automatic pitch parameter input");
        const auto minimum = library.Require<Parameter>(settings.AutoPitchParameter).Minimum;
        const auto range = settings.AutoPitchMaximum - minimum;
        if (range != 0) multiplier = settings.AutoPitchMinimum
            + (input.Parameters(settings.AutoPitchParameter) - minimum) / range * (1 - settings.AutoPitchMinimum);
        if (!std::isfinite(multiplier) || multiplier < 0) throw FormatError("Invalid automatic pitch multiplier");
    }
    return multiplier * PitchRatio(std::clamp(properties.Get(1), -48.0f, 48.0f));
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
SoundPlayback::~SoundPlayback() { if (voice) mixer.Stop(*voice); }
}
