#ifndef OPENFMOD_STUDIO_TIMELINE_RESOURCE_HPP
#define OPENFMOD_STUDIO_TIMELINE_RESOURCE_HPP
#include "OpenFmod/Studio/Timeline/Model.hpp"
#include "OpenFmod/Studio/Timeline/Transition.hpp"
namespace OpenFmod::Studio {
struct TimelineResource {
    Timeline Definition;
    std::vector<Transition> Transitions;
    std::vector<Bank::Guid> Controllers;
};
}
#endif
