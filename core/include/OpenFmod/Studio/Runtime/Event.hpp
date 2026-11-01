#ifndef OPENFMOD_STUDIO_RUNTIME_EVENT_HPP
#define OPENFMOD_STUDIO_RUNTIME_EVENT_HPP
#include "OpenFmod/Studio/Playback/Control/IEventVoice.hpp"
namespace OpenFmod::Studio {
class RuntimeContext;
class EventState;
struct EventRouting;
enum class PlaybackState { Stopped, Playing, Sustaining, Stopping };
class EventInstance final : public IEventVoice, public std::enable_shared_from_this<EventInstance> {
public:
    ~EventInstance() override;
    void Start(const SoundStart& position = {}) override;
    void Stop(bool immediate) override;
    void End(bool cutOff) override;
    bool IsPlaying() const override;
    void SetMix(float gain, float pitch) override;
    void SetParameter(const Bank::Guid& parameter, float value) override;
    void SetSnapshotIntensity(float percent) override;
    void KeyOff();
    PlaybackState GetState() const;
    std::uint32_t GetTimelinePosition() const;
    bool IsOneshot() const;
    const Bank::Guid& GetId() const;
private:
    friend class System;
    friend class RuntimeContext;
    friend class EventState;
    EventInstance(std::shared_ptr<RuntimeContext> context, const Bank::Guid& event, std::shared_ptr<EventRouting> parent, std::optional<Bank::Guid> input);
    std::unique_ptr<EventState> state;
};
}
#endif
