cmake_minimum_required(VERSION 3.12)

project("dbcppp-tiny" VERSION 3.8.0)

# CONFIGURATION

set(CMAKE_CXX_STANDARD 17)
option(build_tests "Build tests" OFF)
option(build_examples "Build examples" OFF)


# DEPENDENCIES & Requirements
# No external dependencies required!


# CREATE LIBRARY

file(GLOB include "include/dbcppp-tiny/*.h")
file(GLOB headers "src/*.h")
file(GLOB sources "src/*.cpp")

add_library(${PROJECT_NAME} STATIC ${include} ${headers} ${sources})


# CONFIGURE LIBRARY

set_target_properties(${PROJECT_NAME} PROPERTIES
    PUBLIC_HEADER "${include}"
    POSITION_INDEPENDENT_CODE ON
)

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/dbcppp-tiny>
    $<INSTALL_INTERFACE:include/dbcppp-tiny>
    include/
)

# INSTALL LIBRARY

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include/dbcppp-tiny
)


# ADDITIONAL: Tests & Examples

if (build_tests)
  add_subdirectory(tests)
endif()

if (build_examples)
  add_subdirectory(examples)
endif()

# PACKAGE (useful for debugging install, use make package)

set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY NO)
set(CPACK_VERBATIM_VARIABLES YES)
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES \\.git/ build/ ".*~$")
include(CPack)
