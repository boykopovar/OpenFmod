#ifndef OPENFMOD_STUDIO_PLAYBACK_FACTORY_HPP
#define OPENFMOD_STUDIO_PLAYBACK_FACTORY_HPP
#include "OpenFmod/Studio/Playback/Sound.hpp"
#include "OpenFmod/Studio/Playback/PlaylistState.hpp"
#include "OpenFmod/Studio/Playback/Control/IEventVoice.hpp"
#include "OpenFmod/Studio/Playback/Control/IEventControl.hpp"
#include <set>
namespace OpenFmod::Studio {
using MixerResolver = std::function<Audio::Mixer&(const Bank::Guid&)>;
using RoutedEventFactory = std::function<std::shared_ptr<IEventVoice>(const Bank::Guid&, const Bank::Guid&)>;
class InstrumentFactory {
public:
    InstrumentFactory(const Library& library, SourceLibrary& sources, const AutomationBindings& bindings, PlaylistState& playlists, std::function<float()> random, std::uint32_t sampleRate, MixerResolver mixers, RoutedEventFactory events, IEventControl& commands, ProgrammerSoundFactory programmer = {});
    InstrumentFactory(const InstrumentFactory&) = delete;
    InstrumentFactory& operator=(const InstrumentFactory&) = delete;
    std::unique_ptr<IPlayback> Create(const Bank::Guid& instrument);
private:
    std::unique_ptr<IPlayback> create(const Bank::Guid& instrument, const Bank::Guid& routing, std::set<Bank::Guid> ancestors);
    const Library& library;
    SourceLibrary& sources;
    const AutomationBindings& bindings;
    PlaylistState& playlists;
    std::function<float()> random;
    std::uint32_t sampleRate;
    MixerResolver mixers;
    RoutedEventFactory events;
    IEventControl& commands;
    ProgrammerSoundFactory programmer;
};
}
#endif
