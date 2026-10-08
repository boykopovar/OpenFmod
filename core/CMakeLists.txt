add_library(OpenFmod STATIC
    src/Binary/Reader.cpp
    src/Bank/Container.cpp
    src/Bank/Catalog.cpp
    src/Bank/Sources.cpp
    src/Sound/Bank.cpp
    src/Sound/Metadata.cpp
    src/Sound/Vorbis/Setup.cpp
    src/Sound/Vorbis/Decoder.cpp
    src/Studio/Playlist.cpp
    src/Studio/Modulation/Property.cpp
    src/Studio/Modulation/Model.cpp
    src/Studio/Modulation/Curve.cpp
    src/Studio/Modulation/Mapping.cpp
)
add_library(OpenFmod::OpenFmod ALIAS OpenFmod)
target_include_directories(OpenFmod PUBLIC include)
target_compile_features(OpenFmod PUBLIC cxx_std_17)
target_link_libraries(OpenFmod PRIVATE vorbis vorbisenc)
set_target_properties(OpenFmod PROPERTIES CXX_EXTENSIONS OFF POSITION_INDEPENDENT_CODE ON)
if(MSVC)
    target_compile_options(OpenFmod PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(OpenFmod PRIVATE -Wall -Wextra -Wpedantic -pedantic-errors)
endif()
