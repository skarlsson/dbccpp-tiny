# Combined CMakeLists.txt for dbcppp-tiny
# Works with both ESP-IDF and standard CMake builds

# Explicit file list (no glob)
set(SOURCES
    "src/attribute_impl.cpp"
    "src/attribute_definition_impl.cpp"
    "src/bit_timing_impl.cpp"
    "src/dbcast2network.cpp"
    "src/message_impl.cpp"
    "src/network_impl.cpp"
    "src/node_impl.cpp"
    "src/signal_impl.cpp"
    "src/signal_group_impl.cpp"
    "src/signal_multiplexer_value_impl.cpp"
    "src/signal_type_impl.cpp"
    "src/value_encoding_description_impl.cpp"
    "src/value_table_impl.cpp"
)

# Check if we're building with ESP-IDF
if(DEFINED ESP_PLATFORM)
    # ESP-IDF build
    idf_component_register(
        SRCS ${SOURCES}
        INCLUDE_DIRS
            "include"
        REQUIRES ""
        PRIV_REQUIRES ""
    )

    # Set C++ standard for ESP-IDF
    target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)

    # Add compile options for ESP-IDF
    target_compile_options(${COMPONENT_LIB} PRIVATE
        -Wno-unused-parameter
        -Wno-sign-compare
        -Wno-unused-variable
    )

else()
    # Standard CMake build (Linux/Mac/Windows)
    cmake_minimum_required(VERSION 3.12)
    project("dbcppp-tiny" VERSION 3.8.0)

    # CONFIGURATION
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    option(BUILD_TESTS "Build tests" ON)
    option(BUILD_EXAMPLES "Build examples" OFF)

    # DEPENDENCIES & Requirements
    # Find glog for logging on Linux
    find_package(glog QUIET)
    if(glog_FOUND)
        message(STATUS "Found glog - will use for logging")
    else()
        message(STATUS "glog not found - logging disabled")
    endif()

    # Find Catch2 for tests (if tests are enabled)
    if(BUILD_TESTS)
        find_package(Catch2 3 QUIET)
        if(Catch2_FOUND)
            message(STATUS "Found Catch2 v3 - will build tests")
        else()
            message(WARNING "Catch2 v3 not found - tests will be disabled")
            set(BUILD_TESTS OFF)
        endif()
    endif()

    # CREATE LIBRARY
    file(GLOB include "include/dbcppp-tiny/*.h")
    file(GLOB headers "src/*.h")

    add_library(${PROJECT_NAME} STATIC ${include} ${headers} ${SOURCES})

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

    # Link with glog if available
    if(glog_FOUND)
        target_link_libraries(${PROJECT_NAME} PUBLIC glog::glog)
        target_compile_definitions(${PROJECT_NAME} PUBLIC USE_GLOG)
    else()
        target_compile_definitions(${PROJECT_NAME} PUBLIC DBCPPP_NO_LOGGING)
    endif()

    # INSTALL LIBRARY
    install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        PUBLIC_HEADER DESTINATION include/dbcppp-tiny
    )

    # ADDITIONAL: Tests & Examples
    if (BUILD_TESTS)
      add_subdirectory(tests)
    endif()

    if (BUILD_EXAMPLES)
      add_subdirectory(examples)
    endif()

    # PACKAGE (useful for debugging install, use make package)
    set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY NO)
    set(CPACK_VERBATIM_VARIABLES YES)
    set(CPACK_SOURCE_GENERATOR "TGZ")
    set(CPACK_SOURCE_IGNORE_FILES \\.git/ build/ ".*~$")
    include(CPack)
endif()