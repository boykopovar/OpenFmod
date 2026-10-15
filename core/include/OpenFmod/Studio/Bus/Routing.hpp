#ifndef OPENFMOD_STUDIO_BUS_ROUTING_HPP
#define OPENFMOD_STUDIO_BUS_ROUTING_HPP
#include "OpenFmod/Bank/Identity.hpp"
namespace OpenFmod::Studio {
struct Routing {
    Bank::Guid Bus{};
    std::uint32_t ChannelLayout{};
    std::uint32_t Flags{};
};
Routing ReadRouting(Binary::Reader& reader);
}
#endif
