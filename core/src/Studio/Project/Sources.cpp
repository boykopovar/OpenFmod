#include "OpenFmod/Studio/Project/Sources.hpp"
#include "OpenFmod/Audio/VorbisSource.hpp"
namespace OpenFmod::Studio {
SourceLibrary::SourceLibrary(const Library& library)
    : library(library), registry(std::make_shared<Sound::Vorbis::SetupRegistry>()) {}
std::shared_ptr<const Audio::PcmData> SourceLibrary::prepareDecoded(const Bank::Guid& wave, std::shared_ptr<const Sound::Sample> sample) {
    auto found = decoded.find(wave);
    if (found == decoded.end()) {
        auto data = std::make_shared<Audio::PcmData>();
        data->Layout = {sample->Rate, sample->Channels, sample->Frames};
        data->Samples = Sound::Vorbis::Decode(*sample, *registry);
        found = decoded.emplace(wave, Decoded{std::move(sample), std::move(data)}).first;
    }
    return found->second.Pcm;
}
void SourceLibrary::Prepare(const Bank::Guid& wave) {
    auto sample = library.Retain<Sound::Sample>(wave);
    const auto mode = library.GetOwner<Sound::Sample>(wave)->Sources.Waves.at(wave).Mode;
    switch (mode) {
        case Bank::LoadingMode::Decoded:
            prepareDecoded(wave, std::move(sample));
            return;
        case Bank::LoadingMode::Compressed:
        case Bank::LoadingMode::Streaming:
            if (!sample->VorbisSetup) throw FormatError("Missing Vorbis setup checksum");
            registry->Resolve(sample->Rate, sample->Channels, *sample->VorbisSetup);
            return;
    }
    throw FormatError("Invalid sound loading mode");
}
std::unique_ptr<Audio::IAudioSource> SourceLibrary::Create(const Bank::Guid& wave) {
    auto sample = library.Retain<Sound::Sample>(wave);
    const auto owner = library.GetOwner<Sound::Sample>(wave);
    const auto mode = owner->Sources.Waves.at(wave).Mode;
    switch (mode) {
        case Bank::LoadingMode::Compressed:
        case Bank::LoadingMode::Streaming:
            return std::make_unique<Audio::VorbisSource>(std::move(sample), registry);
        case Bank::LoadingMode::Decoded: {
            return std::make_unique<Audio::PcmSource>(prepareDecoded(wave, std::move(sample)));
        }
    }
    throw FormatError("Invalid sound loading mode");
}
void SourceLibrary::PruneDecoded() {
    for (auto entry = decoded.begin(); entry != decoded.end();) {
        if (!library.Contains<Sound::Sample>(entry->first)) entry = decoded.erase(entry);
        else {
            entry->second.Sample = library.Retain<Sound::Sample>(entry->first);
            ++entry;
        }
    }
}
}
