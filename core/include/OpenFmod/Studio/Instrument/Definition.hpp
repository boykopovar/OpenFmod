#ifndef OPENFMOD_STUDIO_INSTRUMENT_DEFINITION_HPP
#define OPENFMOD_STUDIO_INSTRUMENT_DEFINITION_HPP
#include "OpenFmod/Bank/Identity.hpp"
#include <string>
#include <variant>
namespace OpenFmod::Studio {
struct WaveInstrument {
    Bank::Guid Source{};
};
struct MultiInstrument {};
struct SilenceInstrument {
    float DurationMilliseconds{};
};
struct ParameterOverride {
    std::uint32_t PropertyId{};
    Bank::Guid Parameter{};
    float Value{};
};
struct EventInstrument {
    Bank::Guid Event{};
    float SnapshotIntensity{};
    std::vector<ParameterOverride> Parameters;
};
struct ProgrammerInstrument {
    std::string Name;
};
struct CommandInstrument {
    std::uint32_t Type{};
    Bank::Guid Target{};
    float Value{};
};
struct InstrumentDefinition {
    Bank::Guid Id{};
    std::variant<WaveInstrument, MultiInstrument, SilenceInstrument, EventInstrument, ProgrammerInstrument, CommandInstrument> Source;
};
InstrumentDefinition ReadInstrumentDefinition(std::string_view type, std::string_view bytes);
}
#endif
