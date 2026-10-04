#ifndef OPENFMOD_SOUND_METADATA_HPP
#define OPENFMOD_SOUND_METADATA_HPP
#include "OpenFmod/Sound/Bank.hpp"
namespace OpenFmod::Sound {
void ReadMetadata(Sample& sample, std::uint32_t type, std::string_view bytes);
}
#endif
