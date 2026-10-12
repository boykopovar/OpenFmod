#ifndef OPENFMOD_STUDIO_INSTRUMENT_MODEL_HPP
#define OPENFMOD_STUDIO_INSTRUMENT_MODEL_HPP
#include "OpenFmod/Studio/Instrument/Definition.hpp"
#include "OpenFmod/Studio/Instrument/Settings.hpp"
#include "OpenFmod/Studio/Modulation/Property.hpp"
#include "OpenFmod/Studio/Playlist.hpp"
namespace OpenFmod::Studio {
struct Instrument {
    InstrumentDefinition Definition;
    InstrumentSettings Settings;
    std::map<std::uint32_t, Property> Properties;
    std::optional<Playlist> Variants;
};
Instrument ReadInstrument(std::string_view bytes, const Bank::Chunk& chunk);
}
#endif
