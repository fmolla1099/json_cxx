#include <cmath>
#include <string>
#include <utility>
#include <vector>
#include "catch.hpp"

#include "../exceptions.h"
#include "../scanner.h"
#include "../sourcepos.h"
#include "../unicode.h"


using std::isinf;
using std::move;
using std::pair;
using std::string;
using std::vector;


vector<Token::Ptr> get_tokens(const string &str) {
    ustring us = u8_decode(str.data());
    Scanner scanner;
    for (auto ch : us) {
        scanner.feed(ch);
    }
    scanner.feed('\0');

    vector<Token::Ptr> ans;
    Token::Ptr tok;
    while ((tok = scanner.pop())) {
        ans.push_back(move(tok));
    }

    REQUIRE(!ans.empty());
    REQUIRE(ans.back()->type == TokenType::END);
    ans.pop_back();
    return ans;
}


vector<Token::Ptr> check_tokens(const string &str, const vector<Token *> &expect) {
    auto tokens = get_tokens(str);
    REQUIRE(tokens.size() == expect.size());
    for (size_t i = 0; i < tokens.size(); ++i) {
        CHECK(*tokens[i] == *expect[i]);
        delete expect[i];
    }
    return tokens;
}


void check_single_char_token(const string &str) {
    auto tokens = get_tokens(str);
    REQUIRE(tokens.size() == str.size());
    for (size_t i = 0; i < tokens.size(); ++i) {
        auto tok = *tokens[i];
        CHECK(tok == Token(static_cast<TokenType>(str[i])));
        CHECK(tok.start == SourcePos(0, i));
        CHECK(tok.end == SourcePos(0, i));
    }
}


void check_tokens_pos(
    const vector<Token::Ptr> &tokens, const vector<pair<SourcePos, SourcePos>> &positions)
{
    REQUIRE(tokens.size() == positions.size());
    for (size_t i = 0; i < tokens.size(); ++i) {
        CHECK(tokens[i]->start == positions[i].first);
        CHECK(tokens[i]->end == positions[i].second);
    }
}


TEST_CASE("Test Scanner basic") {
    check_single_char_token("[]{},:");
    auto tokens = check_tokens(" -12.5e-1 ", {new TokenFloat(-1.25)});
    check_tokens_pos(tokens, {{SourcePos(0, 1), SourcePos(0, 8)}});

    tokens = check_tokens("\"asdf\"", {new TokenString(USTRING("asdf"))});
    check_tokens_pos(tokens, {{SourcePos(0, 0), SourcePos(0, 5)}});

    tokens = check_tokens("null true false", {
        new Token(TokenType::NIL),
        new TokenBool(true),
        new TokenBool(false),
    });
    check_tokens_pos(tokens, {
        {SourcePos(0, 0), SourcePos(0, 3)},
        {SourcePos(0, 5), SourcePos(0, 8)},
        {SourcePos(0, 10), SourcePos(0, 14)},
    });

    tokens = get_tokens("[1.2, {\"asdf\": 5}]");
    check_tokens_pos(tokens, {
        {SourcePos(0, 0), SourcePos(0, 0)},
        {SourcePos(0, 1), SourcePos(0, 3)},
        {SourcePos(0, 4), SourcePos(0, 4)},
        {SourcePos(0, 6), SourcePos(0, 6)},
        {SourcePos(0, 7), SourcePos(0, 12)},
        {SourcePos(0, 13), SourcePos(0, 13)},
        {SourcePos(0, 15), SourcePos(0, 15)},
        {SourcePos(0, 16), SourcePos(0, 16)},
        {SourcePos(0, 17), SourcePos(0, 17)},
    });
}


double get_float(const string &input) {
    auto token = get_tokens(input);
    REQUIRE(token.size() == 1);
    TokenFloat *tok = dynamic_cast<TokenFloat *>(token[0].get());
    REQUIRE(tok != nullptr);
    return tok->value;
}


TEST_CASE("Test scanner number") {
    // overflow
    CHECK(get_float("123123123123123123123123123123") == Approx(1.23123E+29));
    CHECK(isinf(get_float("0.4e006699999999999999999999999999999999999999999999999999999")));
}


void check_token_string(const string &str, const char *expect) {
    check_tokens("\"" + str + "\"", {new TokenString(u8_decode(expect))});
}


TEST_CASE("Test Scanner string") {
    check_token_string("\\b\\f\\n\\r\\t\\\"\\/\\\\", "\b\f\n\r\t\"/\\");
    check_token_string("123啊abc", "123啊abc");
    check_token_string("\\u554a\\u00b1 \\u00b1", "啊± ±");
    check_token_string("\\ud852\\udf62", "𤭢");

    CHECK_THROWS_AS(get_tokens("\""), TokenizerError);
    CHECK_THROWS_AS(get_tokens("\"a"), TokenizerError);
    CHECK_THROWS_AS(get_tokens("\"\\u123z"), TokenizerError);
    CHECK_THROWS_AS(get_tokens("\"\\u123"), TokenizerError);
    CHECK_THROWS_AS(get_tokens("\"\\z\""), TokenizerError);

    // bad surrogate pair
    CHECK_THROWS_AS(get_tokens("\"\\ud852\""), TokenizerError);
    CHECK_THROWS_AS(get_tokens("\"\\ud852.\""), TokenizerError);
    CHECK_THROWS_AS(get_tokens("\"\\ud852\\u000a\""), TokenizerError);
    CHECK_THROWS_AS(get_tokens("\"\\udf62\\u000a\""), TokenizerError);
    CHECK_THROWS_AS(get_tokens("\"\\udf62\""), TokenizerError);
    CHECK_THROWS_AS(get_tokens("\"\\uD800\\n\""), TokenizerError);
}


void check_exception_pos(const string &str, SourcePos start, SourcePos end) {
    REQUIRE_THROWS_AS(get_tokens(str), TokenizerError);
    try {
        get_tokens(str);
    } catch (TokenizerError &exc) {
        CHECK(exc.start == start);
        CHECK(exc.end == end);
    }
}


TEST_CASE("Test Scanner exception") {
    check_exception_pos("1.0ee ", SourcePos(0, 0), SourcePos(0, 4));
    check_exception_pos("asdf ", SourcePos(0, 0), SourcePos(0, 3));
}
