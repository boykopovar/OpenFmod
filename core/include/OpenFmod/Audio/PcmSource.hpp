#ifndef OPENFMOD_AUDIO_PCMSOURCE_HPP
#define OPENFMOD_AUDIO_PCMSOURCE_HPP
#include "OpenFmod/Audio/IAudioSource.hpp"
#include <memory>
#include <vector>
namespace OpenFmod::Audio {
struct PcmData {
    Format Layout;
    std::vector<float> Samples;
};
class PcmSource final : public IAudioSource {
public:
    explicit PcmSource(std::shared_ptr<const PcmData> data);
    Format GetFormat() const override;
    std::size_t Read(float* output, std::size_t frames) override;
    void Seek(std::uint64_t frame) override;
private:
    std::shared_ptr<const PcmData> data;
    std::uint64_t position{};
};
}
#endif
