#ifndef OPENFMOD_STUDIO_TIMELINE_PLAYBACK_TRANSPORT_HPP
#define OPENFMOD_STUDIO_TIMELINE_PLAYBACK_TRANSPORT_HPP
#include "OpenFmod/Studio/Timeline/Playback/Scan.hpp"
#include <array>
namespace OpenFmod::Studio {
enum class TimelineRole { Main, Source, Destination, Transition };
struct TimelineCursor {
    std::uint32_t Position{};
    std::uint64_t Generation{};
    TimelineRole Role{};
    bool Advancing{true};
};
struct TimelineSpan {
    std::uint32_t Duration{};
    std::array<TimelineCursor, 3> Cursors{};
    std::size_t Count{};
    const TransitionSequence* Sequence{};
    std::uint32_t SequencePosition{};
    std::uint32_t DestinationTick{};
};
class TimelineTransport {
public:
    explicit TimelineTransport(const TimelineResource& model);
    void Advance(std::uint32_t ticks, const std::function<float(const Bank::Guid&)>& parameters, const std::function<float()>& random, const std::function<void(const TimelineSpan&)>& consume);
    void Seek(std::uint32_t position);
    void KeyOff();
    void SetStopping(bool value);
    std::uint32_t GetPosition() const;
    bool IsSustaining() const;
private:
    struct SequenceState {
        const Transition* Model{};
        std::uint32_t Position{};
        TimelineCursor Source;
        TimelineCursor Destination;
        std::uint64_t Generation{};
    };
    std::uint64_t nextGeneration();
    void begin(const LogicPoint& point);
    std::uint32_t advanceSequence(std::uint32_t ticks, const std::function<float(const Bank::Guid&)>& parameters, const std::function<float()>& random, const std::function<void(const TimelineSpan&)>& consume);
    const TimelineResource& model;
    TimelineScan scan;
    TimelineScan sourceScan;
    TimelineScan destinationScan;
    TimelineCursor current;
    std::optional<SequenceState> sequence;
    std::optional<std::size_t> sustain;
    std::size_t keyOffs{};
    std::uint64_t generation{1};
    bool includeStart{true};
    bool stopping{};
};
}
#endif
