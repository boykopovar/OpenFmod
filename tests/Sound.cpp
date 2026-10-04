#include "OpenFmod/Sound/Bank.hpp"
#include "OpenFmod/Binary/Reader.hpp"
#include <stdexcept>
#include <string>
void CheckSound() {
    std::string bytes = "FSB5";
    const auto append = [&bytes](std::uint64_t value, unsigned width) {
        for (unsigned index = 0; index < width; ++index) bytes += static_cast<char>((value >> (index * 8)) & 255);
    };
    append(1, 4);
    append(1, 4);
    append(20, 4);
    append(0, 4);
    append(4, 4);
    append(15, 4);
    append(1, 4);
    append(0, 4);
    bytes.append(24, '\0');
    append((std::uint64_t{64} << 34) | (9 << 1) | 1, 8);
    append((11 << 25) | (8 << 1), 4);
    append(0x12345678, 4);
    append(0, 4);
    bytes.append(4, '\0');
    const auto bank = OpenFmod::Sound::ReadBank(bytes);
    if (bank.Samples.size() != 1) throw std::runtime_error("Wrong sample count");
    const auto& sample = bank.Samples.front();
    if (sample.Rate != 48000 || sample.Channels != 1 || sample.Frames != 64 || sample.Data.size() != 4) {
        throw std::runtime_error("Wrong sample parameters");
    }
    if (sample.VorbisSetup != 0x12345678) throw std::runtime_error("Wrong sample setup identity");
    for (std::size_t length = 0; length < bytes.size(); ++length) {
        bool rejected = false;
        try { OpenFmod::Sound::ReadBank(std::string_view(bytes).substr(0, length)); }
        catch (const OpenFmod::FormatError&) { rejected = true; }
        if (!rejected) throw std::runtime_error("Truncated sound bank accepted");
    }
}
