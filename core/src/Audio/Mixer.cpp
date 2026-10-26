#include "OpenFmod/Audio/Mixer.hpp"
#include "OpenFmod/Binary/Reader.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
namespace OpenFmod::Audio {
Mixer::Mixer(std::uint32_t rate) : rate(rate) {
    if (!rate) throw FormatError("Invalid output sample rate");
}
std::uint64_t Mixer::Play(std::unique_ptr<IAudioSource> source, VoiceSettings settings) {
    Voice voice(std::move(source), settings, rate);
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
bool Mixer::IsPlaying(std::uint64_t voice) const {
    std::lock_guard<std::mutex> lock(mutex);
    return voices.find(voice) != voices.end();
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
void Mixer::SetVoiceGain(std::uint64_t voice, float value) {
    std::lock_guard<std::mutex> lock(mutex);
    requireVoice(voice).SetGain(value);
}
void Mixer::SetVoicePitch(std::uint64_t voice, float value) {
    std::lock_guard<std::mutex> lock(mutex);
    requireVoice(voice).SetPitch(value);
}
void Mixer::Render(float* stereo, std::size_t frames) {
    if (frames > std::numeric_limits<std::size_t>::max() / 2 || (frames && !stereo)) throw FormatError("Invalid mixer output buffer");
    if (!frames) return;
    std::lock_guard<std::mutex> lock(mutex);
    std::fill_n(stereo, frames * 2, 0.0f);
    for (auto iterator = voices.begin(); iterator != voices.end();) {
        if (!iterator->second.Mix(stereo, frames, gain).Playing) iterator = voices.erase(iterator);
        else ++iterator;
    }
    for (std::size_t index = 0; index < frames * 2; ++index) {
        if (!std::isfinite(stereo[index])) throw FormatError("Nonfinite mixer output");
    }
}
}
