#ifndef OPENFMOD_STUDIO_EVENT_INSPECTION_HPP
#define OPENFMOD_STUDIO_EVENT_INSPECTION_HPP
#include "OpenFmod/Studio/Project/Library.hpp"
namespace OpenFmod::Studio {
bool IsEventOneshot(const Library& library, const Bank::Guid& event);
}
#endif
