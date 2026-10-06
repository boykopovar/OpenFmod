#ifndef OPENFMOD_SOUND_VORBIS_DECODER_HPP
#define OPENFMOD_SOUND_VORBIS_DECODER_HPP
#include "OpenFmod/Sound/Bank.hpp"
#include "OpenFmod/Sound/Vorbis/Setup.hpp"
#include <memory>
namespace OpenFmod::Sound::Vorbis {
class Decoder {
public:
    Decoder(const Sample& sample, SetupRegistry& registry);
    ~Decoder();
    std::size_t Read(float* output, std::size_t frames);
    std::size_t Position() const;
private:
    class Stream;
    std::unique_ptr<Stream> stream;
};
std::vector<float> Decode(const Sample& sample, SetupRegistry& registry);
}
#endif
