#include "OpenFmod/Audio/Mixer.hpp"
#include <algorithm>
#include <stdexcept>
#include <vector>
namespace {
class Signal final : public OpenFmod::Audio::IAudioSource {
public:
    explicit Signal(std::vector<float> data = {0, 0, 1, 2, 3, 4, 5, 6}, std::size_t* rewinds = nullptr)
        : data(std::move(data)), rewinds(rewinds) {}
    OpenFmod::Audio::Format GetFormat() const override { return {4, 2, data.size() / 2}; }
    std::size_t Read(float* output, std::size_t frames) override {
        const auto count = std::min(frames, data.size() / 2 - position);
        std::copy_n(data.data() + position * 2, count * 2, output);
        position += count;
        return count;
    }
    void Seek(std::uint64_t frame) override {
        if (frame > data.size() / 2) throw std::runtime_error("Invalid test seek");
        if (rewinds && frame < position) ++*rewinds;
        position = static_cast<std::size_t>(frame);
    }
private:
    std::vector<float> data;
    std::size_t* rewinds{};
    std::size_t position{};
};
}
void CheckMixer() {
    using namespace OpenFmod::Audio;
    Mixer mixer(4);
    VoiceSettings settings;
    settings.DelayFrames = 2;
    settings.LoopBegin = 1;
    settings.LoopEnd = 3;
    settings.Repeats = 1;
    const auto id = mixer.Play(std::make_unique<Signal>(), settings);
    std::vector<float> result(16);
    mixer.Render(result.data(), 3);
    if (!mixer.IsPlaying(id)) throw std::runtime_error("Delayed loop stopped early");
    mixer.Render(result.data() + 6, 5);
    const std::vector<float> expected{0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 1, 2, 3, 4, 5, 6};
    if (result != expected || mixer.IsPlaying(id)) throw std::runtime_error("Incorrect voice loop or delay");
    settings = {};
    settings.Pitch = 0.5f;
    const auto slower = mixer.Play(std::make_unique<Signal>(), settings);
    mixer.Render(result.data(), 8);
    const std::vector<float> interpolated{0, 0, 0.5f, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 5, 6};
    if (result != interpolated || mixer.IsPlaying(slower)) throw std::runtime_error("Incorrect voice resampling");
    const auto partitioned = mixer.Play(std::make_unique<Signal>(), settings);
    for (std::size_t frame = 0; frame < 8; ++frame) mixer.Render(result.data() + frame * 2, 1);
    if (result != interpolated || mixer.IsPlaying(partitioned)) throw std::runtime_error("Audio changed with render block size");
    settings.Pitch = 1;
    const auto first = mixer.Play(std::make_unique<Signal>(), settings);
    mixer.Play(std::make_unique<Signal>(), settings);
    mixer.SetGain(0.5f);
    mixer.SetVoiceGain(first, 3);
    mixer.Render(result.data(), 4);
    if (result[2] != 2 || result[3] != 4) throw std::runtime_error("Voice gains were not mixed");
    settings.Repeats = -1;
    settings.LoopBegin = 0;
    settings.LoopEnd = 4;
    const auto loop = mixer.Play(std::make_unique<Signal>(), settings);
    mixer.Render(result.data(), 8);
    if (!mixer.IsPlaying(loop) || !mixer.Stop(loop) || mixer.IsPlaying(loop)) throw std::runtime_error("Loop could not be stopped");
    mixer.Render(result.data(), 8);
    if (std::any_of(result.begin(), result.end(), [](float value) { return value != 0; })) throw std::runtime_error("Stopped voice emitted audio");
    std::size_t rewinds = 0;
    settings = {};
    settings.Pitch = 0.25f;
    mixer.Play(std::make_unique<Signal>(std::vector<float>(4096, 1), &rewinds), settings);
    result.resize(16384);
    mixer.Render(result.data(), 8192);
    if (rewinds) throw std::runtime_error("Resampling rewound a sequential audio stream");
}
