#include "OpenFmod/Audio/Voice.hpp"
#include "OpenFmod/Binary/Reader.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
namespace OpenFmod::Audio {
Voice::Voice(std::unique_ptr<IAudioSource> source, VoiceSettings settings, std::uint32_t outputRate, SpeakerMode outputLayout)
    : source(std::move(source)), settings(settings) {
    if (!this->source) throw FormatError("Missing audio source");
    format = this->source->GetFormat();
    if (!outputRate || !format.Rate || !format.Channels || format.Channels > 8 || format.Frames > (std::uint64_t{1} << 53)) {
        throw FormatError("Unsupported audio source format");
    }
    matrix.emplace(SpeakerLayout(format.Channels), outputLayout);
    outputChannels = matrix->GetOutputChannels();
    endFrame = settings.EndFrame.value_or(format.Frames);
    if (endFrame > format.Frames || settings.StartFrame > endFrame) throw FormatError("Invalid voice playback range");
    if (settings.Repeats < -1 || (settings.Repeats && (settings.LoopBegin >= settings.LoopEnd || settings.LoopEnd > endFrame))) {
        throw FormatError("Invalid audio loop range");
    }
    rateRatio = static_cast<double>(format.Rate) / outputRate;
    SetGain(settings.Gain);
    currentGain = settings.GainRampFrames ? 0 : settings.Gain;
    gainRemaining = settings.GainRampFrames;
    if (gainRemaining) gainIncrement = settings.Gain / gainRemaining;
    SetPitch(settings.Pitch);
    position = static_cast<double>(settings.StartFrame);
    this->source->Seek(settings.StartFrame);
}
void Voice::SetGain(float value) {
    if (!std::isfinite(value) || value < 0) throw FormatError("Invalid voice gain");
    if (settings.Gain == value) return;
    settings.Gain = value;
    gainRemaining = settings.GainRampFrames;
    if (gainRemaining) gainIncrement = (value - currentGain) / gainRemaining;
    else currentGain = value;
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
void Voice::advanceLoop() {
    if (!settings.Repeats || position < settings.LoopEnd) return;
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
void Voice::Finish() {
    if (settings.Repeats != -1) return;
    if (position == settings.LoopEnd) {
        settings.Repeats = 0;
        endFrame = settings.LoopEnd;
        return;
    }
    advanceLoop();
    settings.Repeats = 0;
    endFrame = settings.LoopEnd;
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
std::array<float, 8> Voice::readFrame(std::uint64_t frame) {
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
        std::fill_n(converted.data(), buffered * outputChannels, 0.0f);
        matrix->Add(buffer.data(), converted.data(), buffered);
    }
    const auto offset = static_cast<std::size_t>(frame - bufferBegin) * outputChannels;
    auto& cached = recent[recentIndex];
    recentIndex = (recentIndex + 1) % recent.size();
    cached.Position = frame;
    cached.Valid = true;
    cached.Value.fill(0);
    std::copy_n(converted.data() + offset, outputChannels, cached.Value.data());
    return cached.Value;
}
VoiceProgress Voice::Mix(float* stereo, std::size_t frames, float masterGain, float masterPitch) {
    if (frames && !stereo) throw FormatError("Missing voice output buffer");
    if (!std::isfinite(masterGain) || masterGain < 0) throw FormatError("Invalid master gain");
    if (!std::isfinite(settings.Gain * masterGain)) throw FormatError("Voice gain overflow");
    const auto step = rateStep(masterPitch);
    std::size_t output = static_cast<std::size_t>(std::min<std::uint64_t>(settings.DelayFrames, frames));
    settings.DelayFrames -= output;
    for (; output < frames; ++output) {
        advanceLoop();
        if (position >= endFrame) return {output, false};
        const auto frame = static_cast<std::uint64_t>(position);
        const auto fraction = static_cast<float>(position - frame);
        auto next = frame + 1;
        if (settings.Repeats && next == settings.LoopEnd) next = settings.LoopBegin;
        else if (next == endFrame) next = frame;
        const auto first = readFrame(frame);
        const auto second = fraction ? readFrame(next) : first;
        const auto fade = fadePosition < settings.FadeInFrames ? static_cast<float>(fadePosition++) / settings.FadeInFrames : 1;
        const auto gain = currentGain * masterGain * fade;
        for (std::size_t channel = 0; channel < outputChannels; ++channel) {
            stereo[output * outputChannels + channel] += (first[channel] + (second[channel] - first[channel]) * fraction) * gain;
        }
        if (gainRemaining) currentGain = --gainRemaining ? currentGain + gainIncrement : settings.Gain;
        position += step;
    }
    return {frames, settings.DelayFrames != 0 || settings.Repeats != 0 || position < endFrame};
}
}
