
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
)

file(GLOB header
    "*.h"
)
set(src
    main.cpp
    catch2.cpp
    api_tests.cpp
    dbc_parser_test.cpp
    decoding_test.cpp
    hand_parser_tests.cpp
)
# Exclude standalone test programs:
# test_lexer.cpp
# test_parser_attributes.cpp
# test_parser_basic.cpp
# test_parser_multiplex.cpp
configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/config.h"
)

add_executable(${PROJECT_NAME}_Test ${header} ${src})
set_property(TARGET ${PROJECT_NAME}_Test PROPERTY CXX_STANDARD 17)
add_dependencies(${PROJECT_NAME}_Test ${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME}_Test ${PROJECT_NAME})

add_custom_target(RunTests COMMAND $<TARGET_FILE:${PROJECT_NAME}_Test> "" DEPENDS ${PROJECT_NAME}_Test)
