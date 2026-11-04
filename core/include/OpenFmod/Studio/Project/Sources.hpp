#ifndef OPENFMOD_STUDIO_PROJECT_SOURCES_HPP
#define OPENFMOD_STUDIO_PROJECT_SOURCES_HPP
#include "OpenFmod/Studio/Project/Library.hpp"
#include "OpenFmod/Audio/PcmSource.hpp"
#include "OpenFmod/Sound/Vorbis/Setup.hpp"
namespace OpenFmod::Studio {
class SourceLibrary {
public:
    explicit SourceLibrary(const Library& library);
    void Prepare(const Bank::Guid& wave);
    std::unique_ptr<Audio::IAudioSource> Create(const Bank::Guid& wave);
    void PruneDecoded();
private:
    struct Decoded {
        std::shared_ptr<const Sound::Sample> Sample;
        std::shared_ptr<const Audio::PcmData> Pcm;
    };
    const Library& library;
    std::shared_ptr<Sound::Vorbis::SetupRegistry> registry;
    std::map<Bank::Guid, Decoded> decoded;
    std::shared_ptr<const Audio::PcmData> prepareDecoded(const Bank::Guid& wave, std::shared_ptr<const Sound::Sample> sample);
};
}
#endif
