#include "OpenFmod/Studio/Playback/Multi.hpp"
#include "OpenFmod/Studio/Event/Playback/Instruments.hpp"
#include "../../Data/EventControl.hpp"
#include "../../Data/Bank.hpp"
#include "OpenFmod/Studio/Runtime/System.hpp"
#include "OpenFmod/Studio/Runtime/Event.hpp"
#include "OpenFmod/Studio/Playback/Sound.hpp"
#include "OpenFmod/Studio/Playback/PlaylistState.hpp"
#include "../../Data/Writer.hpp"
#include <stdexcept>
#include "OpenFmod/Audio/PcmSource.hpp"
namespace {
std::string instrument(unsigned char id, bool multi, unsigned mode, bool empty, bool programmer = false) {
    using namespace TestData;
    std::string settings(80, '\0');
    std::string probability;
    AppendFloat(probability, 100);
    settings.replace(36, 4, probability);
    Append(settings, 1, 4);
    Append(settings, 24, 2);
    settings.append(24, '\0');
    if (id != 2) settings[86] = 11;
    settings.append(20, '\0');
    std::string identity(16, '\0');
    identity[0] = static_cast<char>(id);
    if (programmer) { Append(identity, 4, 2); identity += "tone"; }
    else if (!multi) AppendFloat(identity, 7);
    auto body = std::string(programmer ? "PRIT" : multi ? "MUIT" : "SLNI") + Chunk("INST", settings)
        + Chunk(programmer ? "PRIB" : multi ? "MUIB" : "SLNB", identity) + Chunk("LIST", "PRPS" + Chunk("LCNT", std::string(4, '\0')));
    if (multi) {
        std::string playlist;
        Append(playlist, mode, 4);
        Append(playlist, 1, 4);
        Append(playlist, empty ? 0 : 3, 2);
        if (!empty) {
        Append(playlist, 20, 2);
        std::string child(16, '\0');
        child[0] = 2;
        playlist += child;
        AppendFloat(playlist, 1);
        }
        body += Chunk("PLST", playlist);
    }
    return Chunk("LIST", body);
}
std::string event(unsigned char id, unsigned char source, unsigned maximum, unsigned stealing, float cooldown, unsigned flags) {
    using namespace TestData;
    std::string bytes(80, '\0');
    bytes[0] = static_cast<char>(id);
    bytes[64] = 11;
    Append(bytes, maximum, 4);
    Append(bytes, 128, 4);
    Append(bytes, 0, 5);
    Append(bytes, 1, 2);
    Append(bytes, 0, 4);
    AppendFloat(bytes, 0);
    Append(bytes, stealing, 4);
    AppendFloat(bytes, cooldown);
    Append(bytes, flags, 4);
    Append(bytes, 1, 2);
    Append(bytes, 0, 2);
    Append(bytes, 3, 2);
    Append(bytes, 16, 2);
    std::string instrument(16, '\0');
    instrument[0] = static_cast<char>(source);
    bytes += instrument;
    AppendFloat(bytes, 0);
    AppendFloat(bytes, 0);
    return Chunk("LIST", "EVNT" + Chunk("EVTB", bytes) + Chunk("LIST", "PRPS" + Chunk("LCNT", std::string(4, '\0'))));
}
std::string bank(unsigned mode = 0, bool empty = false, unsigned maximum = 4, unsigned stealing = 2, float cooldown = 0, unsigned flags = 0x1800) {
    using namespace TestData;
    std::string version;
    Append(version, 142, 4);
    std::string count;
    Append(count, 1, 4);
    auto body = std::string("PROJ") + Chunk("BNKI", std::string(16, '\0')) + Chunk("SNDH", {}) + Chunk("STDT", {}) + Chunk("LIST", "WAVS");
    body += Chunk("LIST", "MUIS" + Chunk("LCNT", count) + instrument(1, true, mode, empty));
    body += Chunk("LIST", "SLNS" + Chunk("LCNT", count) + instrument(2, false, mode, empty));
    body += Chunk("LIST", "PRIS" + Chunk("LCNT", count) + instrument(3, false, mode, empty, true));
    body += Chunk("LIST", "MBSS" + Chunk("LCNT", count) + Bus(11, 0, "MB"));
    std::string events;
    Append(events, 2, 4);
    body += Chunk("LIST", "EVTS" + Chunk("LCNT", events) + event(4, 1, maximum, stealing, cooldown, flags) + event(5, 3, maximum, stealing, cooldown, flags));
    return Chunk("RIFF", "FEV " + Chunk("FMT ", version) + Chunk("LIST", body));
}
}
void CheckMultiPlayback() {
    using namespace OpenFmod;
    using namespace OpenFmod::Studio;
    Library library;
    library.Load(bank());
    SourceLibrary sources(library);
    AutomationBindings bindings(library, Event{});
    Bank::Guid id{};
    id[0] = 1;
    auto run = [&](std::size_t block) {
        Audio::Mixer mixer(1000);
        auto cursor = std::make_shared<PlaylistCursor>(*library.Require<Instrument>(id).Variants, std::vector<bool>{true});
        std::size_t created = 0;
        MultiPlayback playback(library, id, bindings, [] { return 0.5f; }, cursor, [&](const auto& child) {
            ++created;
            return std::make_unique<SoundPlayback>(library, sources, mixer, child, bindings, [] { return 0.5f; }, 1000);
        });
        playback.Start({0, 0, 2}, {});
        std::uint64_t position = 0;
        std::vector<float> output(block * 2);
        while (playback.IsPlaying()) {
            const auto frames = std::min<std::uint64_t>(block, mixer.FramesUntilNextEnd().value_or(block));
            if (!frames || position > 100) throw std::runtime_error("Playlist stalled at its audio boundary");
            mixer.Render(output.data(), frames);
            position += frames;
            playback.Update({}, static_cast<double>(position));
        }
        if (position != 21 || created != 3) throw std::runtime_error("Playlist repeats introduced a gap or lost an entry");
        playback.Stop(true, static_cast<double>(position));
    };
    run(1);
    run(64);
    Audio::Mixer endingMixer(1000);
    SoundPlayback ending(library, sources, endingMixer, Bank::Guid{2}, bindings, [] { return 0.5f; }, 1000);
    ending.Start({0, 0, -1}, {});
    std::array<float, 14> endingOutput{};
    endingMixer.Render(endingOutput.data(), 3);
    ending.End(false, 3);
    if (endingMixer.FramesUntilNextEnd() != 4) throw std::runtime_error("Asynchronous instrument end cut its active iteration");
    endingMixer.Render(endingOutput.data(), 4);
    if (ending.IsPlaying()) throw std::runtime_error("Asynchronous instrument end kept looping");
    Audio::Mixer scheduledMixer(1000);
    TestData::EventControl commands;
    PlaylistState playlists(library);
    InstrumentFactory factory(library, sources, bindings, playlists, [] { return 0.5f; }, 1000,
        [&](const auto&) -> Audio::Mixer& { return scheduledMixer; },
        [](const auto&, const auto&) -> std::unique_ptr<IEventVoice> { throw std::runtime_error("Unexpected nested event"); }, commands);
    EventInstruments scheduled(library, bindings, factory, [] { return 0.0f; }, [](auto tick, auto, auto) { return tick; });
    const TimelineTrigger first{1, 0, false};
    const TimelineTrigger second{1, 1, false};
    scheduled.Activate(first, id, 0, {}, 0);
    scheduled.Update({}, 0);
    scheduled.Activate(second, id, 0, {}, 0);
    if (scheduled.NextStart()) throw std::runtime_error("Instrument instance limit accepted an overlapping launch");
    scheduled.Update({}, 0);
    scheduledMixer.Render(endingOutput.data(), 3);
    scheduled.Update({}, 144);
    scheduled.Deactivate(first, 144);
    scheduled.Deactivate(second, 144);
    if (scheduledMixer.FramesUntilNextEnd() != 4) throw std::runtime_error("Timeline end cut asynchronous playlist audio");
    scheduledMixer.Render(endingOutput.data(), 4);
    scheduled.Update({}, 336);
    if (scheduled.IsPlaying()) throw std::runtime_error("Scheduled playlist did not finish");
    Audio::Mixer programmerMixer(1000);
    Bank::Guid programmerId{};
    programmerId[0] = 3;
    bool supplied = false;
    SoundPlayback programmer(library, sources, programmerMixer, programmerId, bindings, [] { return 0.5f; }, 1000,
        [&](const std::string& name) {
            if (name != "tone") throw std::runtime_error("Programmer instrument changed its resource name");
            supplied = true;
            auto pcm = std::make_shared<Audio::PcmData>();
            pcm->Layout = {1000, 2, 4};
            pcm->Samples = {1, 1, 2, 2, 3, 3, 4, 4};
            return std::make_unique<Audio::PcmSource>(pcm);
        });
    programmer.Start({48, 0, 0}, {});
    std::array<float, 6> pcm{};
    programmerMixer.Render(pcm.data(), 3);
    if (!supplied || pcm != std::array<float, 6>{2, 2, 3, 3, 4, 4} || programmer.IsPlaying()) {
        throw std::runtime_error("Programmer audio source was not rendered with its requested offset");
    }
    System published(1000);
    published.LoadBank(bank(0, false, 4, 2, 0, 0x1801));
    if (published.GetEvents().size() != 2 || published.GetEvents(Bank::Guid{}).size() != 2) throw std::runtime_error("Public bank events were not enumerated");
    System runtime(1000);
    runtime.LoadBank(bank());
    if (!runtime.GetEvents().empty() || !runtime.GetEvents(Bank::Guid{}).empty()) throw std::runtime_error("Private nested events were published");
    runtime.SetProgrammerSoundFactory([](const auto&) {
        auto pcm = std::make_shared<Audio::PcmData>();
        pcm->Layout = {1000, 2, 4};
        pcm->Samples = {1, 1, 2, 2, 3, 3, 4, 4};
        return std::make_unique<Audio::PcmSource>(pcm);
    });
    auto instance = runtime.CreateInstance(Bank::Guid{5});
    instance->SetMix(0.5f, 1);
    instance->Start();
    std::array<float, 12> rendered{};
    runtime.Render(rendered.data(), 6);
    if (instance->IsPlaying() || rendered != std::array<float, 12>{0.5f, 0.5f, 1, 1, 1.5f, 1.5f, 2, 2, 0, 0}) {
        throw std::runtime_error("Runtime event lost routing, gain, audio or natural completion");
    }
    instance->Start();
    instance->Stop(true);
    runtime.Render(rendered.data(), 6);
    for (auto value : rendered) if (value != 0) throw std::runtime_error("Stopped runtime event emitted audio");
    for (unsigned policy = 0; policy <= 2; ++policy) {
        System limited(1000);
        limited.LoadBank(bank(0, false, 1, policy));
        auto first = limited.CreateInstance(Bank::Guid{4});
        auto second = limited.CreateInstance(Bank::Guid{4});
        first->Start();
        second->Start();
        if (first->IsPlaying() != (policy != 2) || second->IsPlaying() != (policy != 0)) throw std::runtime_error("Event polyphony policy was not applied");
    }
    System cooling(1000);
    cooling.LoadBank(bank(0, false, 4, 2, 0.01f));
    auto coolingFirst = cooling.CreateInstance(Bank::Guid{4});
    auto coolingSecond = cooling.CreateInstance(Bank::Guid{4});
    coolingFirst->Start();
    coolingSecond->Start();
    if (coolingSecond->IsPlaying()) throw std::runtime_error("Event cooldown was not shared across instances");
    std::array<float, 20> silence{};
    cooling.Render(silence.data(), 9);
    coolingSecond->Start();
    if (coolingSecond->IsPlaying()) throw std::runtime_error("Event cooldown expired early");
    cooling.Render(silence.data(), 1);
    coolingSecond->Start();
    if (!coolingSecond->IsPlaying()) throw std::runtime_error("Event cooldown did not expire");
    cooling.UnloadBank(Bank::Guid{});
    if (coolingSecond->IsPlaying()) throw std::runtime_error("Unloaded bank retained active event audio");
    cooling.LoadBank(bank());
    bool rejected = false;
    try { coolingSecond->Start(); } catch (const FormatError&) { rejected = true; }
    if (!rejected) throw std::runtime_error("Reloaded bank revived an invalidated instance");
    cooling.UnloadBank(Bank::Guid{});
    System persistent(1000);
    persistent.LoadBank(bank(0, false, 4, 2, 0, 0x1802));
    auto sustained = persistent.CreateInstance(Bank::Guid{4});
    sustained->Start();
    persistent.Render(silence.data(), 10);
    if (!sustained->IsPlaying()) throw std::runtime_error("Persistent event stopped after its audio ended");
    sustained->End(false);
    persistent.Render(silence.data(), 1);
    if (sustained->IsPlaying()) throw std::runtime_error("Natural ending retained a persistent event");
    for (unsigned mode = 0; mode <= 4; ++mode) {
        Library scopedLibrary;
        scopedLibrary.Load(bank(mode));
        PlaylistState global(scopedLibrary);
        PlaylistState first(scopedLibrary, global);
        PlaylistState second(scopedLibrary, global);
        const bool shared = first.Acquire(id) == second.Acquire(id);
        if (shared != (mode != 0 && mode != 4)) throw std::runtime_error("Incorrect playlist state scope");
    }
    Library emptyLibrary;
    emptyLibrary.Load(bank(0, true));
    PlaylistState state(emptyLibrary);
    AutomationBindings emptyBindings(emptyLibrary, Event{});
    MultiPlayback empty(emptyLibrary, id, emptyBindings, [] { return 0.5f; }, state.Acquire(id), [](const auto&) -> std::unique_ptr<IPlayback> {
        throw std::runtime_error("Empty playlist attempted to construct a child");
    });
    empty.Start({}, {});
    if (empty.IsPlaying()) throw std::runtime_error("Empty playlist remained active");
    empty.Stop(true, 0);
}
