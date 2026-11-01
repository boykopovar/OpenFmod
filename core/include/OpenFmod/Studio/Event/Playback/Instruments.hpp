#ifndef OPENFMOD_STUDIO_EVENT_PLAYBACK_INSTRUMENTS_HPP
#define OPENFMOD_STUDIO_EVENT_PLAYBACK_INSTRUMENTS_HPP
#include "OpenFmod/Studio/Playback/Factory.hpp"
#include "OpenFmod/Studio/Timeline/Playback/Schedule.hpp"
#include <list>
namespace OpenFmod::Studio {
using InstrumentQuantizer = std::function<std::uint64_t(std::uint64_t, std::uint32_t, std::uint32_t)>;
struct InstrumentFrame {
    AutomationInput Input;
    float Gain{1};
};
using InstrumentFrameProvider = std::function<InstrumentFrame(const TimelineTrigger&, const Bank::Guid&)>;
class EventInstruments {
public:
    EventInstruments(const Library& library, const AutomationBindings& bindings, InstrumentFactory& factory, std::function<float()> random, InstrumentQuantizer quantize, InstrumentFrameProvider frames = {});
    void Activate(const TimelineTrigger& key, const Bank::Guid& instrument, std::uint32_t offset, const AutomationInput& input, std::uint64_t tick);
    void Deactivate(const TimelineTrigger& key, std::uint64_t tick);
    void Update(const AutomationInput& input, std::uint64_t tick);
    void Stop(bool immediate, std::uint64_t tick);
    bool IsPlaying() const;
    std::optional<std::uint64_t> NextStart() const;
private:
    struct Trigger {
        std::shared_ptr<const Instrument> Model;
        std::uint64_t Start{};
        std::uint32_t Offset{};
        bool Enabled{};
    };
    struct Instance {
        TimelineTrigger Key;
        std::shared_ptr<const Instrument> Model;
        std::unique_ptr<IPlayback> Playback;
        std::uint64_t Due{};
        std::uint32_t Offset{};
        bool Started{};
    };
    void schedule(const TimelineTrigger& key, const Trigger& trigger, const AutomationInput& input, std::uint64_t tick);
    void end(const TimelineTrigger& key, bool cutOff, std::uint64_t tick);
    void checkTime(std::uint64_t tick);
    float sample();
    const Library& library;
    const AutomationBindings& bindings;
    InstrumentFactory& factory;
    std::function<float()> random;
    InstrumentQuantizer quantize;
    InstrumentFrameProvider frames;
    std::map<TimelineTrigger, Trigger> triggers;
    std::list<Instance> instances;
    std::uint64_t lastTick{};
    bool stopping{};
};
}
#endif
