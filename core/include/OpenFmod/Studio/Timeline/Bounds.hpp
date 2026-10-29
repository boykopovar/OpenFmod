#ifndef OPENFMOD_STUDIO_TIMELINE_BOUNDS_HPP
#define OPENFMOD_STUDIO_TIMELINE_BOUNDS_HPP
#include "OpenFmod/Studio/Timeline/Resource.hpp"
namespace OpenFmod::Studio {
std::uint32_t TimelineEnd(std::uint32_t start, std::uint32_t duration);
const NamedMarker& RequireMarker(const Timeline& timeline, const Bank::Guid& id);
std::uint32_t TimelineDuration(const TimelineResource& timeline);
}
#endif
