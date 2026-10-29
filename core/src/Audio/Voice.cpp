#include "OpenFmod/Audio/Voice.hpp"
#include "OpenFmod/Binary/Reader.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
namespace OpenFmod::Audio {
Voice::Voice(std::unique_ptr<IAudioSource> source, VoiceSettings settings, std::uint32_t outputRate)
    : source(std::move(source)), settings(settings) {
    if (!this->source) throw FormatError("Missing audio source");
    format = this->source->GetFormat();
    if (!outputRate || !format.Rate || !format.Channels || format.Channels > 2 || format.Frames > (std::uint64_t{1} << 53)) {
        throw FormatError("Unsupported audio source format");
    }
    endFrame = settings.EndFrame.value_or(format.Frames);
    if (endFrame > format.Frames || settings.StartFrame > endFrame) throw FormatError("Invalid voice playback range");
    if (settings.Repeats < -1 || (settings.Repeats && (settings.LoopBegin >= settings.LoopEnd || settings.LoopEnd > endFrame))) {
        throw FormatError("Invalid audio loop range");
    }
    rateRatio = static_cast<double>(format.Rate) / outputRate;
    SetGain(settings.Gain);
    SetPitch(settings.Pitch);
    position = static_cast<double>(settings.StartFrame);
    this->source->Seek(settings.StartFrame);
}
void Voice::SetGain(float value) {
    if (!std::isfinite(value) || value < 0) throw FormatError("Invalid voice gain");
    settings.Gain = value;
}
void Voice::SetPitch(float value) {
    if (!std::isfinite(value) || value < 0) throw FormatError("Invalid voice pitch");
    settings.Pitch = value;
}
double Voice::rateStep(float masterPitch) const {
    if (!std::isfinite(masterPitch) || masterPitch <= 0) throw FormatError("Invalid master pitch");
    const auto result = rateRatio * settings.Pitch * masterPitch;
    if (!std::isfinite(result) || result < 0) throw FormatError("Voice rate overflow");
    return result;
}
std::optional<std::uint64_t> Voice::FramesUntilEnd(float masterPitch) const {
    const auto step = rateStep(masterPitch);
    if (settings.Repeats < 0) return std::nullopt;
    const auto remaining = std::max(0.0, static_cast<double>(endFrame) - position
        + static_cast<double>(settings.Repeats) * (settings.LoopEnd - settings.LoopBegin));
    if (!step) return remaining ? std::nullopt : std::optional<std::uint64_t>{settings.DelayFrames};
    const auto frames = std::ceil(remaining / step);
    if (frames >= static_cast<double>(std::numeric_limits<std::uint64_t>::max() - settings.DelayFrames)) {
        throw FormatError("Voice duration overflow");
    }
    return settings.DelayFrames + static_cast<std::uint64_t>(frames);
}
std::array<float, 2> Voice::readFrame(std::uint64_t frame) {
    for (const auto& cached : recent) {
        if (cached.Valid && cached.Position == frame) return cached.Value;
    }
    if (!buffered || frame < bufferBegin || frame >= bufferBegin + buffered) {
        source->Seek(frame);
        bufferBegin = frame;
        const auto count = static_cast<std::size_t>(std::min<std::uint64_t>(512, endFrame - frame));
        buffered = source->Read(buffer.data(), count);
        if (buffered != count) throw FormatError("Audio source duration does not match its data");
        for (std::size_t index = 0; index < buffered * format.Channels; ++index) {
            if (!std::isfinite(buffer[index])) throw FormatError("Nonfinite source sample");
        }
    }
    const auto offset = static_cast<std::size_t>(frame - bufferBegin) * format.Channels;
    auto& cached = recent[recentIndex];
    recentIndex = (recentIndex + 1) % recent.size();
    cached.Position = frame;
    cached.Valid = true;
    if (format.Channels == 1) {
        const auto value = buffer[offset] * 0.7071067811865475244f;
        cached.Value = {value, value};
    } else cached.Value = {buffer[offset], buffer[offset + 1]};
    return cached.Value;
}
VoiceProgress Voice::Mix(float* stereo, std::size_t frames, float masterGain, float masterPitch) {
    if (frames && !stereo) throw FormatError("Missing voice output buffer");
    if (!std::isfinite(masterGain) || masterGain < 0) throw FormatError("Invalid master gain");
    const auto gain = settings.Gain * masterGain;
    if (!std::isfinite(gain)) throw FormatError("Voice gain overflow");
    const auto step = rateStep(masterPitch);
    std::size_t output = static_cast<std::size_t>(std::min<std::uint64_t>(settings.DelayFrames, frames));
    settings.DelayFrames -= output;
    for (; output < frames; ++output) {
        if (settings.Repeats && position >= settings.LoopEnd) {
            const auto length = settings.LoopEnd - settings.LoopBegin;
            const auto traversals = std::floor((position - settings.LoopBegin) / length);
            if (settings.Repeats < 0 || traversals < settings.Repeats) {
                position = settings.LoopBegin + std::fmod(position - settings.LoopBegin, length);
                if (settings.Repeats > 0) settings.Repeats -= static_cast<std::int32_t>(traversals);
            } else {
                position -= static_cast<double>(settings.Repeats) * length;
                settings.Repeats = 0;
            }
        }
        if (position >= endFrame) return {output, false};
        const auto frame = static_cast<std::uint64_t>(position);
        const auto fraction = static_cast<float>(position - frame);
        auto next = frame + 1;
        if (settings.Repeats && next == settings.LoopEnd) next = settings.LoopBegin;
        else if (next == endFrame) next = frame;
        const auto first = readFrame(frame);
        const auto second = fraction ? readFrame(next) : first;
        for (std::size_t channel = 0; channel < 2; ++channel) {
            stereo[output * 2 + channel] += (first[channel] + (second[channel] - first[channel]) * fraction) * gain;
        }
        position += step;
    }
    return {frames, settings.DelayFrames != 0 || settings.Repeats != 0 || position < endFrame};
}
}
