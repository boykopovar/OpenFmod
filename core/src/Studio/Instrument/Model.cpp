#include "OpenFmod/Studio/Instrument/Model.hpp"
namespace OpenFmod::Studio {
Instrument ReadInstrument(std::string_view bytes, const Bank::Chunk& chunk) {
    if (chunk.Id != "LIST") throw FormatError("Expected instrument container");
    std::string_view header;
    if (chunk.Form == "WAIT") header = "WAIB";
    else if (chunk.Form == "MUIT") header = "MUIB";
    else if (chunk.Form == "SLNI") header = "SLNB";
    else if (chunk.Form == "EVIT") header = "EVIB";
    else if (chunk.Form == "PRIT") header = "PRIB";
    else if (chunk.Form == "CMDI") header = "CMDB";
    else throw FormatError("Unsupported instrument container: " + chunk.Form);
    Instrument result;
    result.Definition = ReadInstrumentDefinition(header, chunk.Require(header).Data(bytes));
    result.Settings = ReadInstrumentSettings(chunk.Require("INST").Data(bytes));
    result.Properties = ReadProperties(bytes, chunk.Require("LIST", "PRPS"));
    if (chunk.Form == "MUIT") result.Variants = ReadPlaylist(chunk.Require("PLST").Data(bytes));
    const auto expected = result.Variants ? 4u : 3u;
    if (chunk.Children.size() != expected) throw FormatError("Unexpected instrument component");
    return result;
}
}
