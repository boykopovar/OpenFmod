#ifndef OPENFMOD_AUDIO_IAUDIOSOURCE_HPP
#define OPENFMOD_AUDIO_IAUDIOSOURCE_HPP
#include <cstddef>
#include <cstdint>
namespace OpenFmod::Audio {
struct Format {
    std::uint32_t Rate{};
    std::uint32_t Channels{};
    std::uint64_t Frames{};
};
class IAudioSource {
public:
    virtual ~IAudioSource() = default;
    virtual Format GetFormat() const = 0;
    virtual std::size_t Read(float* output, std::size_t frames) = 0;
    virtual void Seek(std::uint64_t frame) = 0;
};
}
#endif
