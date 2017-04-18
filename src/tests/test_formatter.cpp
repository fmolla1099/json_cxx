#include <sstream>
#include "catch.hpp"

#include "helper.h"
#include "../formatter.h"


using std::ostringstream;


void check_fmt(const string &input, const string &output, FormatOption opt = FormatOption())
{
    Node::Ptr node = parse_string(input);
    auto fmtter = Formatter(opt);
    ostringstream os;
    fmtter.format(os, *node);
    CHECK(os.str() == output);
}


TEST_CASE("Test Formatter simple value") {
    check_fmt("1", "1");
    check_fmt("null", "null");
    check_fmt("true", "true");
    check_fmt("false", "false");
    check_fmt("\"asdf\"", "\"asdf\"");
}


void check_string_fmt(const string &input, const char *expect = nullptr) {
    string quoted = "\"" + input + "\"";
    string expected_str = expect ? ("\"" + string(expect) + "\"") : quoted;
    check_fmt(quoted, expected_str);
    CHECK(*parse_string(quoted) == *parse_string(expected_str));
}


TEST_CASE("Test Formatter string") {
    check_string_fmt("a");
    check_string_fmt("\\n");
    check_string_fmt("啊");
    check_string_fmt("\\u554a", "啊");
}


TEST_CASE("Test Formatter compound value") {
    check_fmt("[]", "[]");
    check_fmt("{}", "{}");
    check_fmt("[1, 2, 3]", "[1, 2, 3]");
    check_fmt("[1, {}, []]", "[1, {}, []]");
    check_fmt("[[[[]]]]", "[[[[]]]]");
    check_fmt("[[[[{}]]]]", "[[[[{}]]]]");
    check_fmt(
        "[[4, 5], 2, 3]",
        "[\n"
        "    [4, 5],\n"
        "    2,\n"
        "    3\n"
        "]"
    );
    check_fmt(
        "{\"a\": 123}",
        "{\n"
        "    \"a\": 123\n"
        "}"
    );
    check_fmt(
        "[{\"a\": 123}]",
        "[{\n"
        "    \"a\": 123\n"
        "}]"
    );
    check_fmt(
        "[{\"a\": 123}, 4]",
        "[\n"
        "    {\n"
        "        \"a\": 123\n"
        "    },\n"
        "    4\n"
        "]"
    );
    check_fmt(
        "{\"a\": {\"b\": null, \"c\": [1, [2, 3]]}, \"d\": [4, 5]}",
        "{\n"
        "    \"a\": {\n"
        "        \"b\": null,\n"
        "        \"c\": [\n"
        "            1,\n"
        "            [2, 3]\n"
        "        ]\n"
        "    },\n"
        "    \"d\": [4, 5]\n"
        "}"
    );
}


TEST_CASE("Test FormatterOption") {
    check_fmt(
        "[[1, 2, 3], 4]",
        "[\n"
        "  [1, 2, 3],\n"
        "  4\n"
        "]",
        FormatOption().indent(2)
    );
}
