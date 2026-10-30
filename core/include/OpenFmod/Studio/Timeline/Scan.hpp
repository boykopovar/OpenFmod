#ifndef OPENFMOD_STUDIO_TIMELINE_SCAN_HPP
#define OPENFMOD_STUDIO_TIMELINE_SCAN_HPP
#include "OpenFmod/Studio/Timeline/Resource.hpp"
#include "OpenFmod/Studio/Timeline/TempoGrid.hpp"
#include <set>
namespace OpenFmod::Studio {
struct LogicPoint {
    std::uint32_t Tick{};
    std::optional<std::size_t> Sustain;
    std::optional<std::size_t> Transition;
    std::optional<std::size_t> LeadTransition;
};
struct TimelineWindow {
    std::uint32_t Begin{};
    std::uint32_t End{};
    bool IncludeStart{};
    bool LeadsOnly{};
    bool Stopping{};
    std::size_t KeyOffs{};
};
class TimelineScan {
public:
    explicit TimelineScan(const TimelineResource& timeline);
    std::optional<LogicPoint> Find(const TimelineWindow& window, const std::function<float(const Bank::Guid&)>& parameter, const std::function<float()>& random);
    void Reset();
private:
    std::optional<std::uint32_t> candidate(std::size_t index, std::uint32_t begin, std::uint32_t end, const std::function<float()>& random);
    static bool roll(float probability, const std::function<float()>& random);
    const TimelineResource& timeline;
    TempoGrid tempo;
    std::set<std::size_t> rejected;
};
}
#endif
