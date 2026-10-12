add_library(OpenFmod STATIC
    src/Binary/Reader.cpp
    src/Bank/Container.cpp
    src/Bank/Catalog.cpp
    src/Bank/Sources.cpp
    src/Sound/Bank.cpp
    src/Sound/Metadata.cpp
    src/Sound/Vorbis/Setup.cpp
    src/Sound/Vorbis/Decoder.cpp
    src/Audio/VorbisSource.cpp
    src/Audio/Voice.cpp
    src/Audio/Mixer.cpp
    src/Studio/Playlist.cpp
    src/Studio/Condition.cpp
    src/Studio/Automation/Controller.cpp
    src/Studio/Event/Model.cpp
    src/Studio/Timeline/Model.cpp
    src/Studio/Timeline/Transition.cpp
    src/Studio/Timeline/TempoGrid.cpp
    src/Studio/Modulation/Property.cpp
    src/Studio/Modulation/Model.cpp
    src/Studio/Modulation/EnvelopeState.cpp
    src/Studio/Modulation/FollowerState.cpp
    src/Studio/Modulation/RandomState.cpp
    src/Studio/Modulation/Curve.cpp
    src/Studio/Modulation/Mapping.cpp
)
add_library(OpenFmod::OpenFmod ALIAS OpenFmod)
target_include_directories(OpenFmod PUBLIC include)
target_compile_features(OpenFmod PUBLIC cxx_std_17)
find_package(Threads REQUIRED)
target_link_libraries(OpenFmod PRIVATE vorbis vorbisenc Threads::Threads)
set_target_properties(OpenFmod PROPERTIES CXX_EXTENSIONS OFF POSITION_INDEPENDENT_CODE ON)
if(MSVC)
    target_compile_options(OpenFmod PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(OpenFmod PRIVATE -Wall -Wextra -Wpedantic -pedantic-errors)
endif()
