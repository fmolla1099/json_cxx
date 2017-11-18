cmake_minimum_required(VERSION 3.8)
project(json_cxx)

set(CMAKE_CXX_STANDARD 11)


set(CATCH_SRC src/tests/catch_main.cpp)

set(TEST_UNICODE_SRC
    ${CATCH_SRC}
    src/tests/test_unicode.cpp
    src/unicode.cpp)

set(TEST_SCANNER_SRC
    ${CATCH_SRC}
    src/tests/test_scanner.cpp
    src/scanner.cpp
    src/node.cpp
    src/formatter.cpp
    src/sourcepos.cpp
    src/unicode.cpp)

set(JSON_CXX_SRC
    src/formatter.cpp
    src/parser.cpp
    src/scanner.cpp
    src/node.cpp
    src/sourcepos.cpp
    src/unicode.cpp
    src/exceptions.cpp)

set(TEST_PARSER_SRC
    ${CATCH_SRC}
    src/tests/test_parser.cpp
    ${JSON_CXX_SRC})

set(TEST_FORMATTER_SRC
    ${CATCH_SRC}
    src/tests/test_formatter.cpp
    src/tests/helper.cpp
    ${JSON_CXX_SRC})

set(VALIDATOR_OPTION_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tests/validator_option.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tests/validator_option.h)
set(VALIDATOR_OPTION_ARGGEN ${CMAKE_CURRENT_SOURCE_DIR}/src/tests/validator_option.arggen)

add_custom_command(
    OUTPUT ${VALIDATOR_OPTION_SRC}
    COMMAND arggen ${VALIDATOR_OPTION_ARGGEN}
    MAIN_DEPENDENCY ${VALIDATOR_OPTION_ARGGEN}
    COMMENT "generating src with arggen"
    VERBATIM
)

set(VALIDATOR_SRC
    src/tests/validator.cpp
    ${VALIDATOR_OPTION_SRC}
    ${JSON_CXX_SRC})


add_executable(test_unicode ${TEST_UNICODE_SRC})
add_executable(test_scanner ${TEST_SCANNER_SRC})
add_executable(test_parser ${TEST_PARSER_SRC})
add_executable(test_formatter ${TEST_FORMATTER_SRC})

add_executable(validator ${VALIDATOR_SRC})
