#ifndef OPENFMOD_STUDIO_TIMELINE_TRANSITION_HPP
#define OPENFMOD_STUDIO_TIMELINE_TRANSITION_HPP
#include "OpenFmod/Studio/Condition.hpp"
#include "OpenFmod/Studio/Timeline/Sequence.hpp"
#include "OpenFmod/Bank/Container.hpp"
namespace OpenFmod::Studio {
struct Transition {
    Bank::Guid Id{};
    Bank::Guid Destination{};
    std::uint32_t Start{};
    std::uint32_t End{};
    Condition Trigger{std::string_view{}};
    std::uint32_t QuantizationMode{};
    std::uint32_t QuantizationInterval{};
    float Probability{};
    std::uint32_t Flags{};
    std::optional<TransitionSequence> Sequence;
};
Transition ReadTransition(std::string_view bytes);
Transition ReadTransition(std::string_view bytes, const Bank::Chunk& chunk);
}
#endif
