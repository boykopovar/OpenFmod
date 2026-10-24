#include "OpenFmod/Studio/Effect/Configuration.hpp"
#include "OpenFmod/Studio/Effect/Values.hpp"
namespace OpenFmod::Studio {
bool CompressorSidechainEnabled(const std::vector<EffectParameter>& values) {
    const EffectValues parameters(values, 7);
    const auto data = parameters.Get<std::string>(5);
    if (data.empty()) return false;
    Binary::Reader reader(data);
    const auto enabled = reader.Unsigned(4);
    reader.Finish();
    if (enabled > 1) throw FormatError("Invalid compressor sidechain flag");
    return enabled != 0;
}
Audio::CompressorSettings CompressorConfiguration(const std::vector<EffectParameter>& values) {
    const EffectValues parameters(values, 7);
    CompressorSidechainEnabled(values);
    return {parameters.Get<float>(0), parameters.Get<float>(1), parameters.Get<float>(2),
        parameters.Get<float>(3), parameters.Get<float>(4), parameters.Get<bool>(6)};
}
Audio::ReverbSettings ReverbConfiguration(const std::vector<EffectParameter>& values) {
    const EffectValues parameters(values, 13);
    return {parameters.Get<float>(0), parameters.Get<float>(1), parameters.Get<float>(2),
        parameters.Get<float>(3), parameters.Get<float>(4), parameters.Get<float>(5),
        parameters.Get<float>(6), parameters.Get<float>(7), parameters.Get<float>(8),
        parameters.Get<float>(9), parameters.Get<float>(10), parameters.Get<float>(11), parameters.Get<float>(12)};
}
Audio::ThreeBandSettings ThreeBandConfiguration(const std::vector<EffectParameter>& values) {
    const EffectValues parameters(values, 6);
    const auto slope = parameters.Get<std::int32_t>(5);
    if (slope < 0) throw FormatError("Invalid equalizer slope");
    return {{parameters.Get<float>(0), parameters.Get<float>(1), parameters.Get<float>(2)},
        parameters.Get<float>(3), parameters.Get<float>(4), static_cast<std::uint32_t>(slope)};
}
Audio::EqualizerBand EqualizerConfiguration(const std::vector<EffectParameter>& values, std::size_t band) {
    const EffectValues parameters(values, 20);
    if (band >= 5) throw FormatError("Invalid equalizer band");
    const auto type = parameters.Get<std::int32_t>(band * 4);
    return {static_cast<Audio::FilterType>(type), parameters.Get<float>(band * 4 + 1),
        parameters.Get<float>(band * 4 + 2), parameters.Get<float>(band * 4 + 3)};
}
}
