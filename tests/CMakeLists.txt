add_executable(OpenFmodChecks Container.cpp Catalog.cpp Sound.cpp)
target_link_libraries(OpenFmodChecks PRIVATE OpenFmod::OpenFmod)
set_target_properties(OpenFmodChecks PROPERTIES CXX_EXTENSIONS OFF)
add_test(NAME OpenFmodChecks COMMAND OpenFmodChecks)
