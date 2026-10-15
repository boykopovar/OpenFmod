#include "OpenFmod/Studio/Bus/Model.hpp"
namespace OpenFmod::Studio {
Routing ReadRouting(Binary::Reader& reader) {
    Binary::Reader data(reader.Read(static_cast<std::size_t>(reader.Unsigned(2))));
    Routing result;
    result.Bus = Bank::ReadGuid(data);
    result.ChannelLayout = static_cast<std::uint32_t>(data.Unsigned(4));
    result.Flags = static_cast<std::uint32_t>(data.Unsigned(4));
    data.Finish();
    return result;
}
namespace {
std::vector<BusEffect> readEffects(Binary::Reader& reader) {
    std::vector<BusEffect> result;
    for (const auto& id : Bank::ReadGuids(reader)) result.push_back({id, 0});
    return result;
}
void readLayouts(Binary::Reader& reader, std::vector<BusEffect>& effects) {
    const auto values = reader.Elements(4);
    if (values.size() != effects.size()) throw FormatError("Bus effect layouts count mismatch");
    for (std::size_t index = 0; index < values.size(); ++index) {
        Binary::Reader data(values[index]);
        effects[index].ChannelLayout = static_cast<std::uint32_t>(data.Unsigned(4));
    }
}
}
Bus ReadBus(std::string_view bytes, const Bank::Chunk& chunk) {
    if (chunk.Id != "LIST" || chunk.Children.size() != 3) throw FormatError("Invalid bus container");
    Bus result;
    std::string_view header;
    if (chunk.Form == "IBUS") { result.Kind = BusKind::Input; header = "IBSB"; }
    else if (chunk.Form == "GBUS") { result.Kind = BusKind::Group; header = "GBSB"; }
    else if (chunk.Form == "MBUS") { result.Kind = BusKind::Master; header = "MBSB"; }
    else if (chunk.Form == "RBUS") { result.Kind = BusKind::Return; header = "RBSB"; }
    else throw FormatError("Unsupported bus container: " + chunk.Form);
    Binary::Reader identity(chunk.Require(header).Data(bytes));
    result.Id = Bank::ReadGuid(identity);
    result.Routing = ReadRouting(identity);
    identity.Finish();
    Binary::Reader reader(chunk.Require("BUS ").Data(bytes));
    result.Flags = static_cast<std::uint32_t>(reader.Unsigned(4));
    result.ChannelLayout = static_cast<std::uint32_t>(reader.Unsigned(4));
    result.PreEffects = readEffects(reader);
    result.PostEffects = readEffects(reader);
    result.Levels = ReadMixControls(reader);
    result.MaximumInstances = static_cast<std::int32_t>(reader.Signed(4));
    result.StealingMode = static_cast<std::int32_t>(reader.Signed(4));
    readLayouts(reader, result.PreEffects);
    readLayouts(reader, result.PostEffects);
    result.ChannelIndex = static_cast<std::int32_t>(reader.Signed(4));
    result.ChannelOrder = static_cast<std::uint32_t>(reader.Unsigned(4));
    reader.Finish();
    result.Properties = ReadProperties(bytes, chunk.Require("LIST", "PRPS"));
    if (result.MaximumInstances < 1 || result.StealingMode < 0 || result.StealingMode > 4
        || result.ChannelOrder > 8) throw FormatError("Invalid bus settings");
    return result;
}
}
