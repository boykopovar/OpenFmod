#ifndef OPENFMOD_BANK_SOURCES_HPP
#define OPENFMOD_BANK_SOURCES_HPP
#include "OpenFmod/Bank/Identity.hpp"
#include "OpenFmod/Bank/Container.hpp"
#include "OpenFmod/Sound/Bank.hpp"
#include <map>
namespace OpenFmod::Bank {
enum class LoadingMode { Compressed, Decoded, Streaming };
struct Wave {
    std::uint32_t BankIndex;
    std::uint32_t SampleIndex;
    LoadingMode Mode;
};
struct Sources {
    std::vector<Sound::Bank> Banks;
    std::map<Guid, Wave> Waves;
    const Sound::Sample& Require(const Guid& id) const;
};
Sources ReadSources(std::string_view bytes, const Chunk& container);
}
#endif
