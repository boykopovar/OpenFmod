#ifndef OPENFMOD_TEST_BANK_HPP
#define OPENFMOD_TEST_BANK_HPP
#include "Writer.hpp"
#include <vector>
namespace TestData {
inline std::string Bus(unsigned char identity, unsigned char parent, const std::string& kind, const std::vector<unsigned char>& effects = {}) {
    using namespace TestData;
    std::string header(16, '\0');
    header[0] = static_cast<char>(identity);
    Append(header, 24, 2);
    std::string route(16, '\0');
    route[0] = static_cast<char>(parent);
    header += route;
    Append(header, 0, 8);
    std::string values;
    Append(values, 0, 4);
    Append(values, 3, 4);
    Append(values, effects.empty() ? 0 : effects.size() * 2 + 1, 2);
    if (!effects.empty()) Append(values, 16, 2);
    for (auto effect : effects) {
        std::string identity(16, '\0');
        identity[0] = static_cast<char>(effect);
        values += identity;
    }
    Append(values, 0, 2);
    Append(values, 8, 2);
    AppendFloat(values, 0);
    AppendFloat(values, 0);
    Append(values, 0, 2);
    Append(values, 32, 4);
    Append(values, 0, 4);
    Append(values, effects.empty() ? 0 : effects.size() * 2 + 1, 2);
    if (!effects.empty()) Append(values, 4, 2);
    for (std::size_t index = 0; index < effects.size(); ++index) Append(values, 3, 4);
    Append(values, 0, 2);
    Append(values, 0xffffffff, 4);
    Append(values, 0, 4);
    return TestData::Chunk("LIST", kind + "US" + TestData::Chunk(kind + "SB", header) + TestData::Chunk("BUS ", values)
        + TestData::Chunk("LIST", "PRPS" + TestData::Chunk("LCNT", std::string(4, '\0'))));
}
}
#endif
