#include "OpenFmod/Audio/Dynamics/Compressor.hpp"
#include <cmath>
#include <stdexcept>
#include <vector>
void CheckDynamics() {
    using namespace OpenFmod::Audio;
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
    Compressor whole(48000, settings), split(48000, settings);
    auto a = sidechain;
    auto b = a;
    whole.Process(a.data(), 24000);
    for (std::size_t i = 0; i < 24000; ++i) split.Process(b.data() + 2 * i, 1);
    if (a != b) throw std::runtime_error("Compression depends on block boundaries");
}
