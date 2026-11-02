#include "OpenFmod/Audio/Mixer.hpp"
#include "OpenFmod/Audio/Graph.hpp"
#include "OpenFmod/Studio/Playback/Sound.hpp"
#include <algorithm>
#include <cmath>
#include "OpenFmod/Audio/PcmSource.hpp"
#include <stdexcept>
#include <vector>
namespace {
class Offset final : public OpenFmod::Audio::IAudioProcessor {
public:
    void Process(float* stereo, std::size_t frames) override {
        for (std::size_t index = 0; index < frames * 2; ++index) stereo[index] += 1;
    }
};
class Visit final : public OpenFmod::Audio::IAudioProcessor {
public:
    Visit(std::vector<int>& visits, int id) : visits(visits), id(id) {}
    void Process(float*, std::size_t) override { visits.push_back(id); }
private:
    std::vector<int>& visits;
    int id;
};
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
    {
        using namespace OpenFmod::Audio;
        auto impulse = std::make_shared<PcmData>();
        impulse->Layout = {48000, 6, 1};
        impulse->Samples = {1, 0, 0, 0, 0, 0};
        Mixer stereo(48000);
        stereo.Play(std::make_unique<PcmSource>(impulse));
        std::array<float, 2> output{};
        stereo.Render(output.data(), 1);
        if (std::abs(output[0] - 0.72000295f) > 0.000001f || output[1] != 0) throw std::runtime_error("Surround front channel was not downmixed");
        auto mono = std::make_shared<PcmData>();
        mono->Layout = {48000, 1, 1};
        mono->Samples = {1};
        Mixer surround(48000, SpeakerMode::Surround51);
        surround.Play(std::make_unique<PcmSource>(mono));
        std::array<float, 6> expanded{};
        surround.Render(expanded.data(), 1);
        for (std::size_t channel = 0; channel < expanded.size(); ++channel) {
            if (std::abs(expanded[channel] - (channel == 3 ? 0 : 0.4472136f)) > 0.000001f) throw std::runtime_error("Mono surround projection changed channel power");
        }
    }
    using namespace OpenFmod::Audio;
    OpenFmod::Sound::Sample looped;
    looped.Rate = 48000;
    looped.Frames = 8;
    looped.Repeat = OpenFmod::Sound::Loop{2, 4};
    auto loopPosition = OpenFmod::Studio::SoundPosition(looped, {7, 0, 2});
    if (loopPosition.StartFrame != 4 || loopPosition.Repeats != 1 || loopPosition.LoopEnd != 5) {
        throw std::runtime_error("Sound offset did not consume completed loop iterations");
    }
    Voice frozen(std::make_unique<Signal>(), VoiceSettings{}, 4);
    frozen.SetPitch(0);
    std::array<float, 4> frozenOutput{};
    frozen.Mix(frozenOutput.data(), 2, 1);
    if (frozen.FramesUntilEnd()) throw std::runtime_error("Zero pitch consumed source time");
    frozen.SetPitch(1);
    if (frozen.FramesUntilEnd() != 4) throw std::runtime_error("Resuming pitch lost source position");
    Graph pitched(4);
    const auto parentPitch = pitched.AddBus(0);
    pitched.SetPitch(parentPitch, 0.5f);
    const auto childPitch = pitched.AddBus(parentPitch);
    pitched.SetPitch(childPitch, 4);
    pitched.GetMixer(childPitch).Play(std::make_unique<Signal>());
    if (pitched.FramesUntilNextEnd() != 2) throw std::runtime_error("Bus pitch inheritance lost voice duration");
    std::array<float, 4> pitchOutput{};
    pitched.Render(pitchOutput.data(), 1);
    pitched.SetPitch(parentPitch, 0.25f);
    if (pitched.FramesUntilNextEnd() != 2) throw std::runtime_error("Changing ancestor pitch did not reach active voices");
    pitched.Render(pitchOutput.data(), 2);
    if (pitchOutput != std::array<float, 4>{3, 4, 5, 6}) throw std::runtime_error("Changing bus pitch reset voice playback");
    VoiceSettings clipped;
    clipped.StartFrame = 1;
    clipped.EndFrame = 3;
    clipped.DelayFrames = 2;
    Voice voice(std::make_unique<Signal>(), clipped, 4);
    if (voice.FramesUntilEnd() != 4) throw std::runtime_error("Incorrect delayed voice endpoint");
    std::array<float, 12> clippedOutput{};
    const auto progress = voice.Mix(clippedOutput.data(), 6, 1);
    if (progress.Playing || progress.Frames != 4 || clippedOutput != std::array<float, 12>{0, 0, 0, 0, 1, 2, 3, 4, 0, 0, 0, 0}) {
        throw std::runtime_error("Voice did not report its exact clipped ending");
    }
    if (voice.FramesUntilEnd() != 0) throw std::runtime_error("Completed voice retained a duration");
    clipped.StartFrame = 0;
    clipped.EndFrame = 4;
    clipped.DelayFrames = 3;
    clipped.LoopBegin = 1;
    clipped.LoopEnd = 3;
    clipped.Repeats = 2;
    Voice repeated(std::make_unique<Signal>(), clipped, 4);
    if (repeated.FramesUntilEnd() != 11) throw std::runtime_error("Incorrect looped voice endpoint");
    repeated.SetPitch(2);
    if (repeated.FramesUntilEnd() != 7) throw std::runtime_error("Pitch change retained the previous endpoint");
    Graph ordered(4);
    const auto source = ordered.AddBus(0);
    const auto consumer = ordered.AddBus(0);
    std::vector<int> visits;
    ordered.AddProcessor(source, ProcessingStage::AfterFader, std::make_unique<Visit>(visits, 1));
    ordered.AddProcessor(consumer, ProcessingStage::BeforeFader, std::make_unique<Visit>(visits, 2));
    ordered.AddDependency(source, consumer);
    bool rejected = false;
    try { ordered.AddDependency(consumer, source); }
    catch (const std::runtime_error&) { rejected = true; }
    std::array<float, 2> silent{};
    ordered.Render(silent.data(), 1);
    if (!rejected || visits != std::vector<int>{1, 2}) throw std::runtime_error("Audio dependency order or rollback failed");
    ordered.RemoveDependency(source, consumer);
    visits.clear();
    ordered.Render(silent.data(), 1);
    if (visits != std::vector<int>{2, 1}) throw std::runtime_error("Removed audio dependency retained its order");
    Graph graph(4);
    const auto group = graph.AddBus(0);
    const auto child = graph.AddBus(group);
    graph.SetGain(group, 0.5f);
    graph.SetGain(child, 0.25f);
    graph.GetMixer(child).Play(std::make_unique<Signal>(std::vector<float>(8, 1)));
    graph.GetMixer(group).Play(std::make_unique<Signal>(std::vector<float>(8, 1)));
    std::vector<float> graphOutput(8);
    graph.Render(graphOutput.data(), 4);
    if (graphOutput != std::vector<float>(8, 0.625f)) throw std::runtime_error("Incorrect hierarchical bus mixing");
    graph.AddProcessor(child, ProcessingStage::BeforeFader, std::make_unique<Offset>());
    graph.AddProcessor(child, ProcessingStage::AfterFader, std::make_unique<Offset>());
    graph.Render(graphOutput.data(), 4);
    if (graphOutput != std::vector<float>(8, 0.625f)) throw std::runtime_error("Incorrect bus processor order");
    graph.RemoveBus(child);
    graph.RemoveBus(group);
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
    for (std::size_t consumed : {2, 4, 5}) {
        const auto ending = mixer.Play(std::make_unique<Signal>(), settings);
        mixer.Render(result.data(), consumed);
        mixer.Finish(ending);
        const auto remaining = consumed % 4 == 0 ? 0 : 4 - consumed % 4;
        if (mixer.FramesUntilNextEnd() != remaining) throw std::runtime_error("Loop end lost the current iteration boundary");
        mixer.Render(result.data(), remaining);
        if (mixer.IsPlaying(ending)) throw std::runtime_error("Finished loop continued repeating");
    }
    const auto loop = mixer.Play(std::make_unique<Signal>(), settings);
    mixer.Render(result.data(), 8);
    if (!mixer.IsPlaying(loop) || !mixer.Stop(loop) || mixer.IsPlaying(loop)) throw std::runtime_error("Loop could not be stopped");
    mixer.Render(result.data(), 8);
    if (std::any_of(result.begin(), result.end(), [](float value) { return value != 0; })) throw std::runtime_error("Stopped voice emitted audio");
    settings = {};
    settings.StartFrame = 2;
    mixer.SetGain(1);
    const auto offset = mixer.Play(std::make_unique<Signal>(), settings);
    mixer.Render(result.data(), 2);
    if (result[0] != 3 || result[1] != 4 || result[2] != 5 || result[3] != 6 || mixer.IsPlaying(offset)) {
        throw std::runtime_error("Incorrect source offset playback");
    }
    std::size_t rewinds = 0;
    settings = {};
    settings.Pitch = 0.25f;
    mixer.Play(std::make_unique<Signal>(std::vector<float>(4096, 1), &rewinds), settings);
    result.resize(16384);
    mixer.Render(result.data(), 8192);
    if (rewinds) throw std::runtime_error("Resampling rewound a sequential audio stream");
}
