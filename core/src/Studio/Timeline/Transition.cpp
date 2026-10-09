#include "OpenFmod/Studio/Timeline/Transition.hpp"
#include <cmath>
namespace OpenFmod::Studio {
Transition ReadTransition(std::string_view bytes) {
    Binary::Reader reader(bytes);
    Transition result;
    result.Id = Bank::ReadGuid(reader);
    result.Destination = Bank::ReadGuid(reader);
    result.Start = static_cast<std::uint32_t>(reader.Unsigned(4));
    result.End = static_cast<std::uint32_t>(reader.Unsigned(4));
    const auto conditionSize = static_cast<std::size_t>(reader.Unsigned(4));
    result.Trigger = Condition(reader.Read(conditionSize));
    result.QuantizationMode = static_cast<std::uint32_t>(reader.Unsigned(4));
    result.QuantizationInterval = static_cast<std::uint32_t>(reader.Unsigned(4));
    result.Probability = reader.Float();
    result.Flags = static_cast<std::uint32_t>(reader.Unsigned(4));
    if (result.Start > result.End) throw FormatError("Reversed transition interval");
    if (!std::isfinite(result.Probability) || result.Probability < 0 || result.Probability > 100) {
        throw FormatError("Invalid transition probability");
    }
    reader.Finish();
    return result;
}
}
