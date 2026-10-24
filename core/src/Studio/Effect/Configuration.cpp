#include "OpenFmod/Studio/Effect/Configuration.hpp"
namespace OpenFmod::Studio {
namespace {
void requireCount(const std::vector<EffectParameter>& values, std::size_t count) {
    if (values.size() != count) throw FormatError("Incorrect effect parameter count");
}
template<class TValue>
TValue require(const std::vector<EffectParameter>& values, std::size_t index) {
    const auto value = std::get_if<TValue>(&values.at(index));
    if (!value) throw FormatError("Incorrect effect parameter type");
    return *value;
}
}
bool CompressorSidechainEnabled(const std::vector<EffectParameter>& values) {
    requireCount(values, 7);
    const auto data = require<std::string>(values, 5);
    if (data.empty()) return false;
    Binary::Reader reader(data);
    const auto enabled = reader.Unsigned(4);
    reader.Finish();
    if (enabled > 1) throw FormatError("Invalid compressor sidechain flag");
    return enabled != 0;
}
Audio::CompressorSettings CompressorConfiguration(const std::vector<EffectParameter>& values) {
    requireCount(values, 7);
    CompressorSidechainEnabled(values);
    return {require<float>(values, 0), require<float>(values, 1), require<float>(values, 2),
        require<float>(values, 3), require<float>(values, 4), require<bool>(values, 6)};
}
Audio::ReverbSettings ReverbConfiguration(const std::vector<EffectParameter>& values) {
    requireCount(values, 13);
    return {require<float>(values, 0), require<float>(values, 1), require<float>(values, 2),
        require<float>(values, 3), require<float>(values, 4), require<float>(values, 5),
        require<float>(values, 6), require<float>(values, 7), require<float>(values, 8),
        require<float>(values, 9), require<float>(values, 10), require<float>(values, 11), require<float>(values, 12)};
}
Audio::ThreeBandSettings ThreeBandConfiguration(const std::vector<EffectParameter>& values) {
    requireCount(values, 6);
    const auto slope = require<std::int32_t>(values, 5);
    if (slope < 0) throw FormatError("Invalid equalizer slope");
    return {{require<float>(values, 0), require<float>(values, 1), require<float>(values, 2)},
        require<float>(values, 3), require<float>(values, 4), static_cast<std::uint32_t>(slope)};
}
Audio::EqualizerBand EqualizerConfiguration(const std::vector<EffectParameter>& values, std::size_t band) {
    requireCount(values, 20);
    if (band >= 5) throw FormatError("Invalid equalizer band");
    const auto type = require<std::int32_t>(values, band * 4);
    return {static_cast<Audio::FilterType>(type), require<float>(values, band * 4 + 1),
        require<float>(values, band * 4 + 2), require<float>(values, band * 4 + 3)};
}
}
