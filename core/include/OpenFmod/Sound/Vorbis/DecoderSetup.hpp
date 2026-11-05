#ifndef OPENFMOD_SOUND_VORBIS_DECODERSETUP_HPP
#define OPENFMOD_SOUND_VORBIS_DECODERSETUP_HPP
#include "OpenFmod/Sound/Vorbis/Setup.hpp"
#include "OpenFmod/Sound/Vorbis/State.hpp"
namespace OpenFmod::Sound::Vorbis {
class DecoderSetup {
public:
    explicit DecoderSetup(const Headers& headers);
    void Initialize(State& decoder) const;
private:
    mutable State state;
    mutable std::mutex mutex;
};
}
#endif
