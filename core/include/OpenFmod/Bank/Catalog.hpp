#ifndef OPENFMOD_BANK_CATALOG_HPP
#define OPENFMOD_BANK_CATALOG_HPP
#include "OpenFmod/Bank/Identity.hpp"
#include <string>
#include <string_view>
#include <vector>
namespace OpenFmod::Bank {
struct CatalogEntry {
    Guid Id{};
    std::string Path;
};
std::vector<CatalogEntry> ReadCatalog(std::string_view bytes);
}
#endif
