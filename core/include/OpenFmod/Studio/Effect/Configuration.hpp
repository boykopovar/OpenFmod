#ifndef OPENFMOD_STUDIO_EFFECT_CONFIGURATION_HPP
#define OPENFMOD_STUDIO_EFFECT_CONFIGURATION_HPP
#include "OpenFmod/Studio/Effect/Model.hpp"
#include "OpenFmod/Audio/Dynamics/Compressor.hpp"
#include "OpenFmod/Audio/Filters/MultibandEq.hpp"
#include "OpenFmod/Audio/Filters/ThreeBandEq.hpp"
#include "OpenFmod/Audio/Reverb/Reverb.hpp"
namespace OpenFmod::Studio {
bool CompressorSidechainEnabled(const std::vector<EffectParameter>& values);
Audio::CompressorSettings CompressorConfiguration(const std::vector<EffectParameter>& values);
Audio::ReverbSettings ReverbConfiguration(const std::vector<EffectParameter>& values);
Audio::ThreeBandSettings ThreeBandConfiguration(const std::vector<EffectParameter>& values);
Audio::EqualizerBand EqualizerConfiguration(const std::vector<EffectParameter>& values, std::size_t band);
}
#endif
