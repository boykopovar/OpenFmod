#ifndef OPENFMOD_SOUND_VORBIS_DECODER_HPP
#define OPENFMOD_SOUND_VORBIS_DECODER_HPP
#include "OpenFmod/Sound/Bank.hpp"
#include "OpenFmod/Sound/Vorbis/Setup.hpp"
namespace OpenFmod::Sound::Vorbis {
std::vector<float> Decode(const Sample& sample, SetupRegistry& registry);
}
#endif
