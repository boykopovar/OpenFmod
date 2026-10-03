#ifndef OPENFMOD_BANK_CONTAINER_HPP
#define OPENFMOD_BANK_CONTAINER_HPP
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>
namespace OpenFmod::Bank {
struct Chunk {
    std::string Id;
    std::string Form;
    std::size_t Offset{};
    std::size_t Size{};
    std::vector<Chunk> Children;
    const Chunk& Require(std::string_view id, std::string_view form = {}) const;
};
Chunk ReadContainer(std::string_view bytes);
}
#endif
