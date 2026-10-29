#ifndef OPENFMOD_AUDIO_SILENCESOURCE_HPP
#define OPENFMOD_AUDIO_SILENCESOURCE_HPP
#include "OpenFmod/Audio/IAudioSource.hpp"
namespace OpenFmod::Audio {
class SilenceSource final : public IAudioSource {
public:
    explicit SilenceSource(Format format);
    Format GetFormat() const override;
    std::size_t Read(float* output, std::size_t frames) override;
    void Seek(std::uint64_t frame) override;
private:
    Format format;
    std::uint64_t position{};
};
}
#endif
