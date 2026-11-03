#include <array>
#include "OpenFmod/Audio/Dynamics/SignalTap.hpp"
#include "OpenFmod/Audio/Dynamics/Compressor.hpp"
#include <cmath>
#include <stdexcept>
#include <vector>
void CheckDynamics() {
    using namespace OpenFmod::Audio;
    SignalTap meter;
    const std::array<float, 8> measured{1, -1, 0.5f, -0.5f, 0, 0, 0, 0};
    std::array<float, 8> copied{};
    meter.SetGain(0.5f);
    meter.Process(measured.data(), copied.data(), 2, SpeakerMode::Quad);
    const auto level = meter.GetMeter();
    if (copied != measured || level.Peak != 0.5f || level.Energy != 0.3125f) throw std::runtime_error("Sidechain meter downmixed or averaged its source channels");
    CompressorSettings settings;
    settings.ThresholdDecibels = -20;
    settings.Ratio = 2;
    Compressor linked(48000, settings);
    std::vector<float> signal(96000, 1);
    linked.Process(signal.data(), 48000);
    const float expected = std::pow(200.0f, -0.25f);
    if (std::abs(signal.back() - expected) > 0.0001f) throw std::runtime_error("Incorrect linked compression ratio");
    settings.Linked = false;
    Compressor independent(48000, settings);
    signal.assign(96000, 1);
    independent.Process(signal.data(), 48000);
    if (std::abs(signal.back() - std::pow(100.0f, -0.25f)) > 0.0001f) {
        throw std::runtime_error("Incorrect independent channel compression");
    }
    independent.Reset();
    signal.assign(96000, 0.01f);
    const std::vector<float> sidechain(48000, 1);
    independent.ProcessWithSidechain(signal.data(), 48000, sidechain.data(), 1);
    if (std::abs(signal.back() - 0.01f * std::pow(100.0f, -0.25f)) > 0.000001f) {
        throw std::runtime_error("External signal did not control compression");
    }
    for (std::size_t channels : {1u, 6u, 8u}) {
        Compressor combined(48000, settings);
        std::vector<Compressor> individual(channels, Compressor(48000, settings));
        std::vector<float> input(512 * channels);
        for (std::size_t index = 0; index < input.size(); ++index) input[index] = std::sin(static_cast<float>(index) * 0.17f);
        auto output = input;
        combined.Process(output.data(), 512, channels);
        for (std::size_t channel = 0; channel < channels; ++channel) {
            std::vector<float> mono(512);
            for (std::size_t frame = 0; frame < 512; ++frame) mono[frame] = input[frame * channels + channel];
            individual[channel].Process(mono.data(), 512, 1);
            for (std::size_t frame = 0; frame < 512; ++frame) {
                if (mono[frame] != output[frame * channels + channel]) throw std::runtime_error("Independent compression linked channel envelopes");
            }
        }
    }
    Compressor whole(48000, settings), split(48000, settings);
    auto a = sidechain;
    auto b = a;
    whole.Process(a.data(), 24000);
    for (std::size_t i = 0; i < 24000; ++i) split.Process(b.data() + 2 * i, 1);
    if (a != b) throw std::runtime_error("Compression depends on block boundaries");
}
