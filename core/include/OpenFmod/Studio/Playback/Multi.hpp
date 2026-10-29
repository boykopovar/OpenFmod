#ifndef OPENFMOD_STUDIO_PLAYBACK_MULTI_HPP
#define OPENFMOD_STUDIO_PLAYBACK_MULTI_HPP
#include "OpenFmod/Studio/Playback/Controls.hpp"
#include "OpenFmod/Studio/Playback/IPlayback.hpp"
namespace OpenFmod::Studio {
using PlaybackFactory = std::function<std::unique_ptr<IPlayback>(const Bank::Guid&)>;
class MultiPlayback final : public IPlayback {
public:
    MultiPlayback(const Library& library, const Bank::Guid& instrument, const AutomationBindings& bindings, std::function<float()> random, std::shared_ptr<PlaylistCursor> cursor, PlaybackFactory factory);
    void Start(const SoundStart& start, const AutomationInput& input) override;
    void Update(const AutomationInput& input, double milliseconds, float parentGain = 1, float parentPitch = 1) override;
    void Stop(bool immediate, double milliseconds) override;
    bool IsPlaying() const override;
private:
    void next(const AutomationInput& input, double milliseconds, const SoundStart& offset);
    const Library& library;
    std::shared_ptr<const Instrument> model;
    InstrumentControls controls;
    std::function<float()> random;
    std::shared_ptr<PlaylistCursor> cursor;
    PlaybackFactory factory;
    std::unique_ptr<IPlayback> child;
    double childStart{};
    double lastUpdate{};
    std::int32_t repeats{};
    bool started{};
    bool releasing{};
    bool finished{};
};
}
#endif
