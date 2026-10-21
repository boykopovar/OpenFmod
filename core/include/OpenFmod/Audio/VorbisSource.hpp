#ifndef OPENFMOD_AUDIO_VORBISSOURCE_HPP
#define OPENFMOD_AUDIO_VORBISSOURCE_HPP
#include "OpenFmod/Audio/IAudioSource.hpp"
#include "OpenFmod/Sound/Vorbis/Decoder.hpp"
namespace OpenFmod::Audio {
class VorbisSource final : public IAudioSource {
public:
    VorbisSource(std::shared_ptr<const Sound::Sample> sample, std::shared_ptr<Sound::Vorbis::SetupRegistry> registry);
    Format GetFormat() const override;
    std::size_t Read(float* output, std::size_t frames) override;
    void Seek(std::uint64_t frame) override;
private:
    std::shared_ptr<const Sound::Sample> sample;
    std::shared_ptr<Sound::Vorbis::SetupRegistry> registry;
    std::unique_ptr<Sound::Vorbis::Decoder> decoder;
    std::vector<float> discard;
};
}
#endif
