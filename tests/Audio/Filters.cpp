#include "OpenFmod/Audio/Filters/ThreeBandEq.hpp"
#include "OpenFmod/Audio/Filters/MultibandEq.hpp"
#include "OpenFmod/Binary/Reader.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>
namespace {
std::vector<float> signal() {
    std::vector<float> result(12000);
    for (std::size_t i = 0; i < result.size() / 2; ++i) {
        result[i * 2] = std::sin(static_cast<float>(i) * 0.07f);
        result[i * 2 + 1] = std::cos(static_cast<float>(i) * 0.41f);
    }
    return result;
}
}
void CheckFilters() {
    using namespace OpenFmod::Audio;
    for (std::uint32_t slope = 0; slope < 3; ++slope) {
        ThreeBandSettings settings;
        settings.Slope = slope;
        settings.GainDecibels = {-4, 2, -8};
        ThreeBandEq whole(48000, settings);
        ThreeBandEq split(48000, settings);
        auto a = signal();
        auto b = a;
        whole.Process(a.data(), 6000);
        split.Process(b.data(), 17);
        split.Process(b.data() + 34, 5983);
        if (a != b) throw std::runtime_error("Equalizer output depends on block boundaries");
        settings.GainDecibels = {-8, -2, -4};
        settings.LowCrossover = 600;
        settings.HighCrossover = 3200;
        whole.Configure(settings);
        split.Configure(settings);
        a = signal();
        b = a;
        whole.Process(a.data(), 6000);
        for (std::size_t i = 0; i < 6000; ++i) split.Process(b.data() + i * 2, 1);
        if (a != b) throw std::runtime_error("Equalizer automation depends on block boundaries");
        settings.GainDecibels = {-80, -80, -80};
        whole.Configure(settings);
        whole.Process(a.data(), 6000);
        if (std::any_of(a.begin() + 512, a.end(), [](float value) { return value != 0; })) {
            throw std::runtime_error("Muted equalizer bands leak audio");
        }
    }
    for (int type = 0; type <= static_cast<int>(FilterType::HighPass6); ++type) {
        MultibandEq whole(48000), split(48000);
        EqualizerBand band{static_cast<FilterType>(type), 2300, 1.2f, -7};
        whole.Configure(0, band);
        split.Configure(0, band);
        auto a = signal(), b = a;
        whole.Process(a.data(), 6000);
        split.Process(b.data(), 137);
        split.Process(b.data() + 274, 5863);
        if (a != b) throw std::runtime_error("Multiband equalizer depends on block boundaries");
        band.Frequency = 4100;
        whole.Configure(0, band, 256);
        split.Configure(0, band, 256);
        a = signal();
        b = a;
        whole.Process(a.data(), 6000);
        for (std::size_t i = 0; i < 6000; ++i) split.Process(b.data() + i * 2, 1);
        if (a != b) throw std::runtime_error("Multiband coefficient ramp depends on block boundaries");
    }
    ThreeBandEq isolated(48000);
    auto stereo = signal();
    for (std::size_t i = 1; i < stereo.size(); i += 2) stereo[i] = 0;
    isolated.Process(stereo.data(), stereo.size() / 2);
    for (std::size_t i = 1; i < stereo.size(); i += 2) {
        if (stereo[i] != 0) throw std::runtime_error("Equalizer leaks between channels");
    }
    ThreeBandSettings invalid;
    invalid.Slope = 3;
    try { isolated.Configure(invalid); }
    catch (const OpenFmod::FormatError&) { return; }
    throw std::runtime_error("Invalid equalizer slope accepted");
}
