#include "OpenFmod/Studio/Project/Sources.hpp"
#include "OpenFmod/Audio/VorbisSource.hpp"
namespace OpenFmod::Studio {
SourceLibrary::SourceLibrary(const Library& library)
    : library(library), registry(std::make_shared<Sound::Vorbis::SetupRegistry>()) {}
std::unique_ptr<Audio::IAudioSource> SourceLibrary::Create(const Bank::Guid& wave) {
    auto sample = library.Retain<Sound::Sample>(wave);
    const auto owner = library.GetOwner<Sound::Sample>(wave);
    const auto mode = owner->Sources.Waves.at(wave).Mode;
    switch (mode) {
        case Bank::LoadingMode::Compressed:
        case Bank::LoadingMode::Streaming:
            return std::make_unique<Audio::VorbisSource>(std::move(sample), registry);
        case Bank::LoadingMode::Decoded: {
            auto found = decoded.find(wave);
            if (found == decoded.end() || found->second.Sample != sample) {
                auto data = std::make_shared<Audio::PcmData>();
                data->Layout = {sample->Rate, sample->Channels, sample->Frames};
                data->Samples = Sound::Vorbis::Decode(*sample, *registry);
                found = decoded.insert_or_assign(wave, Decoded{std::move(sample), std::move(data)}).first;
            }
            return std::make_unique<Audio::PcmSource>(found->second.Pcm);
        }
    }
    throw FormatError("Invalid sound loading mode");
}
void SourceLibrary::ClearDecoded() { decoded.clear(); }
}
