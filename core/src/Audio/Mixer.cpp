#include "OpenFmod/Audio/Mixer.hpp"
#include "OpenFmod/Binary/Reader.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
namespace OpenFmod::Audio {
Mixer::Mixer(std::uint32_t rate, SpeakerMode layout) : rate(rate), layout(layout), channels(Speakers(layout).GetChannels()) {
    if (!rate) throw FormatError("Invalid output sample rate");
}
std::uint64_t Mixer::Play(std::unique_ptr<IAudioSource> source, VoiceSettings settings) {
    Voice voice(std::move(source), settings, rate, layout);
    std::lock_guard<std::mutex> lock(mutex);
    if (!nextId) throw FormatError("Audio voice identifier overflow");
    const auto id = nextId++;
    voices.emplace(id, std::move(voice));
    return id;
}
bool Mixer::Stop(std::uint64_t voice) {
    std::lock_guard<std::mutex> lock(mutex);
    return voices.erase(voice) != 0;
}
void Mixer::Finish(std::uint64_t voice) {
    std::lock_guard<std::mutex> lock(mutex);
    requireVoice(voice).Finish();
}
bool Mixer::IsPlaying(std::uint64_t voice) const {
    std::lock_guard<std::mutex> lock(mutex);
    const auto found = voices.find(voice);
    return found != voices.end() && found->second.FramesUntilEnd(pitch) != std::optional<std::uint64_t>{0};
}
void Mixer::SetGain(float value) {
    if (!std::isfinite(value) || value < 0) throw FormatError("Invalid mixer gain");
    std::lock_guard<std::mutex> lock(mutex);
    gain = value;
}
Voice& Mixer::requireVoice(std::uint64_t voice) {
    const auto found = voices.find(voice);
    if (found == voices.end()) throw FormatError("Unknown audio voice");
    return found->second;
}
void Mixer::SetPitch(float value) {
    if (!std::isfinite(value) || value <= 0) throw FormatError("Invalid mixer pitch");
    std::lock_guard<std::mutex> lock(mutex);
    pitch = value;
}
void Mixer::SetVoiceGain(std::uint64_t voice, float value) {
    std::lock_guard<std::mutex> lock(mutex);
    requireVoice(voice).SetGain(value);
}
void Mixer::SetVoicePitch(std::uint64_t voice, float value) {
    std::lock_guard<std::mutex> lock(mutex);
    requireVoice(voice).SetPitch(value);
}
std::optional<std::uint64_t> Mixer::FramesUntilNextEnd() const {
    std::lock_guard<std::mutex> lock(mutex);
    std::optional<std::uint64_t> result;
    for (const auto& [id, voice] : voices) {
        const auto end = voice.FramesUntilEnd(pitch);
        if (end && (!result || *end < *result)) result = end;
        static_cast<void>(id);
    }
    return result;
}
void Mixer::Render(float* stereo, std::size_t frames) {
    if (frames > std::numeric_limits<std::size_t>::max() / channels || (frames && !stereo)) throw FormatError("Invalid mixer output buffer");
    std::lock_guard<std::mutex> lock(mutex);
    if (frames) std::fill_n(stereo, frames * channels, 0.0f);
    for (auto iterator = voices.begin(); iterator != voices.end();) {
        if (!iterator->second.Mix(stereo, frames, gain, pitch).Playing) iterator = voices.erase(iterator);
        else ++iterator;
    }
    for (std::size_t index = 0; index < frames * channels; ++index) {
        if (!std::isfinite(stereo[index])) throw FormatError("Nonfinite mixer output");
    }
}
}
