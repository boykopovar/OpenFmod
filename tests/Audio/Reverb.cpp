#include "OpenFmod/Audio/Reverb/Reverb.hpp"
#include <cmath>
#include <stdexcept>
#include <vector>
void CheckReverb() {
    using namespace OpenFmod::Audio;
    ReverbSettings settings;
    settings.DryDecibels = -80;
    Reverb whole(48000, settings), blocks(48000, settings);
    std::vector<float> first(96000), second;
    first[0] = 1;
    second = first;
    whole.Process(first.data(), first.size() / 2);
    for (std::size_t frame = 0; frame < second.size() / 2; frame += 127) {
        blocks.Process(second.data() + frame * 2, std::min<std::size_t>(127, second.size() / 2 - frame));
    }
    if (first != second) throw std::runtime_error("Reverb changes with render block size");
    double energy = 0;
    for (const auto value : first) {
        if (!std::isfinite(value)) throw std::runtime_error("Nonfinite reverb impulse response");
        energy += value * value;
    }
    if (energy <= 0 || energy >= 10) throw std::runtime_error("Invalid reverb tail energy");
    whole.Reset();
    std::fill(first.begin(), first.end(), 0);
    whole.Process(first.data(), first.size() / 2);
    for (const auto value : first) {
        if (value != 0) throw std::runtime_error("Reverb reset retained audio");
    }
    settings.DryDecibels = 0;
    settings.WetDecibels = -80;
    whole.Configure(settings);
    whole.Process(first.data(), 256);
    std::fill(first.begin(), first.end(), 0.25f);
    whole.Process(first.data(), first.size() / 2);
    for (const auto value : first) {
        if (value != 0.25f) throw std::runtime_error("Muted reverb changed dry audio");
    }
}
