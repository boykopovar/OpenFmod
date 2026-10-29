#include "OpenFmod/Studio/Playback/Multi.hpp"
#include <cmath>
#include <limits>
namespace OpenFmod::Studio {
MultiPlayback::MultiPlayback(const Library& library, const Bank::Guid& instrument, const AutomationBindings& bindings, std::function<float()> random, std::shared_ptr<PlaylistCursor> cursor, PlaybackFactory factory)
    : library(library), model(library.Retain<Instrument>(instrument)), controls(library, *model, bindings, random),
      random(std::move(random)), cursor(std::move(cursor)), factory(std::move(factory)) {
    if (!std::holds_alternative<MultiInstrument>(model->Definition.Source) || !model->Variants) throw FormatError("Multi playback requires a playlist instrument");
    if (!this->random || !this->cursor || !this->factory) throw FormatError("Missing playlist playback dependency");
}
void MultiPlayback::next(const AutomationInput& input, double milliseconds, const SoundStart& offset) {
    const auto mode = model->Variants->PlayMode;
    const auto index = cursor->Next(mode == 1 || mode == 2 ? random() : 0);
    if (!index || *index >= model->Variants->Entries.size()) throw FormatError("Invalid instrument playlist selection");
    const auto& id = model->Variants->Entries[*index].Id;
    const auto& settings = library.Require<Instrument>(id).Settings;
    if (offset.OffsetTicks > std::numeric_limits<std::uint32_t>::max() - settings.SourceOffsetTicks) throw FormatError("Playlist source offset overflow");
    auto playback = factory(id);
    if (!playback) throw FormatError("Playback factory returned an empty instrument");
    playback->Start({settings.SourceOffsetTicks + offset.OffsetTicks, offset.OffsetPercent, settings.LoopCount}, input);
    child = std::move(playback);
    childStart = milliseconds;
}
void MultiPlayback::Start(const SoundStart& start, const AutomationInput& input) {
    if (started) throw FormatError("Playlist playback has already started");
    if (start.Repeats < -1 || !std::isfinite(start.OffsetPercent) || start.OffsetPercent < 0 || start.OffsetPercent > 100) throw FormatError("Invalid playlist start position");
    if (model->Variants->Entries.empty()) {
        started = true;
        finished = true;
        return;
    }
    controls.Update(input, 0);
    next(input, 0, start);
    repeats = start.Repeats;
    started = true;
    Update(input, 0);
}
void MultiPlayback::Update(const AutomationInput& input, double milliseconds, float parentGain, float parentPitch) {
    if (!started) throw FormatError("Playlist playback has not started");
    if (!std::isfinite(milliseconds) || milliseconds < lastUpdate || !std::isfinite(parentGain) || parentGain < 0
        || !std::isfinite(parentPitch) || parentPitch < 0) throw FormatError("Invalid playlist playback update");
    lastUpdate = milliseconds;
    if (finished) return;
    controls.Update(input, milliseconds);
    if (releasing && controls.IsFinished(milliseconds)) {
        Stop(true, milliseconds);
        return;
    }
    std::size_t empty = 0;
    while (!child->IsPlaying()) {
        if (releasing || repeats == 0) {
            finished = true;
            return;
        }
        if (repeats > 0) --repeats;
        else if (++empty > model->Variants->Entries.size()) throw FormatError("Infinite playlist made no playback progress");
        next(input, milliseconds, {});
    }
    child->Update(input, milliseconds - childStart, parentGain * controls.GetGain(), parentPitch * controls.GetPitch(input));
}
void MultiPlayback::Stop(bool immediate, double milliseconds) {
    if (!started) throw FormatError("Playlist playback has not started");
    if (!std::isfinite(milliseconds) || milliseconds < lastUpdate) throw FormatError("Invalid playlist stop time");
    if (finished) return;
    if (immediate) {
        child->Stop(true, milliseconds - childStart);
        finished = true;
    } else if (!releasing) {
        controls.Release(milliseconds);
        releasing = true;
        repeats = 0;
        if (controls.IsFinished(milliseconds)) Stop(true, milliseconds);
    }
}
bool MultiPlayback::IsPlaying() const { return started && !finished && (child->IsPlaying() || (!releasing && repeats != 0)); }
}
