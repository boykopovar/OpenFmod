#ifndef OPENFMOD_STUDIO_MODULATION_MODEL_HPP
#define OPENFMOD_STUDIO_MODULATION_MODEL_HPP
#include "OpenFmod/Bank/Identity.hpp"
#include <variant>
namespace OpenFmod::Studio {
struct Envelope {
    float Initial{};
    float Peak{};
    float Sustain{};
    float Final{};
    float AttackMilliseconds{};
    float HoldMilliseconds{};
    float DecayMilliseconds{};
    float ReleaseMilliseconds{};
    std::array<float, 3> Curvature{};
};
struct RandomModulation {
    float Amount{};
};
struct EnvelopeFollower {
    float Amount{};
    float MinimumDecibels{};
    float MaximumDecibels{};
    float AttackMilliseconds{};
    float ReleaseMilliseconds{};
    bool RootMeanSquare{};
};
struct Modulation {
    Bank::Guid Id{};
    Bank::Guid Owner{};
    std::uint32_t PropertyId{};
    std::uint32_t ValueMode{};
    std::variant<Envelope, RandomModulation, EnvelopeFollower> Parameters;
};
Modulation ReadModulation(std::string_view bytes);
}
#endif
