#include "OpenFmod/Audio/VorbisSource.hpp"
#include "OpenFmod/Binary/Reader.hpp"
#include <algorithm>
namespace OpenFmod::Audio {
VorbisSource::VorbisSource(Sound::Sample sample, std::shared_ptr<Sound::Vorbis::SetupRegistry> registry)
    : sample(std::move(sample)), registry(std::move(registry)) {
    if (!this->registry) throw FormatError("Missing Vorbis setup registry");
    decoder = std::make_unique<Sound::Vorbis::Decoder>(this->sample, *this->registry);
    discard.resize(512 * this->sample.Channels);
}
Format VorbisSource::GetFormat() const { return {sample.Rate, sample.Channels, sample.Frames}; }
std::size_t VorbisSource::Read(float* output, std::size_t frames) { return decoder->Read(output, frames); }
void VorbisSource::Seek(std::uint64_t frame) {
    if (frame > sample.Frames) throw FormatError("Audio seek exceeds sample duration");
    if (frame < decoder->Position()) decoder = std::make_unique<Sound::Vorbis::Decoder>(sample, *registry);
    while (frame > decoder->Position()) {
        const auto count = static_cast<std::size_t>(std::min<std::uint64_t>(512, frame - decoder->Position()));
        if (decoder->Read(discard.data(), count) != count) throw FormatError("Audio stream ended during seek");
    }
}
}
