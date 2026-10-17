#ifndef OPENFMOD_STUDIO_TIMELINE_SEQUENCE_HPP
#define OPENFMOD_STUDIO_TIMELINE_SEQUENCE_HPP
#include "OpenFmod/Bank/Identity.hpp"
namespace OpenFmod::Studio {
struct ScheduledInstrument {
    Bank::Guid Instrument{};
    std::uint32_t Start{};
    std::uint32_t Duration{};
};
struct CurveBinding {
    Bank::Guid Target{};
    Bank::Guid Curve{};
};
struct TransitionSequence {
    std::uint32_t Duration{};
    std::vector<CurveBinding> Curves;
    std::vector<ScheduledInstrument> Triggered;
    std::vector<ScheduledInstrument> TimeLocked;
    std::uint32_t SourceOffset{};
    std::uint32_t DestinationOffset{};
    std::vector<CurveBinding> SourceFades;
    std::vector<CurveBinding> DestinationFades;
    Bank::Guid FadeMapping{};
    std::vector<CurveBinding> CurveReplacements;
};
std::vector<ScheduledInstrument> ReadSchedule(Binary::Reader& reader);
TransitionSequence ReadTransitionSequence(std::string_view bytes);
}
#endif
