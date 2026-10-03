#ifndef OPENFMOD_BANK_CATALOG_HPP
#define OPENFMOD_BANK_CATALOG_HPP
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
namespace OpenFmod::Bank {
using Guid = std::array<std::uint8_t, 16>;
struct CatalogEntry {
    Guid Id{};
    std::string Path;
};
std::vector<CatalogEntry> ReadCatalog(std::string_view bytes);
}
#endif
