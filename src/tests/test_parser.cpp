#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include "catch.hpp"

#include "../exceptions.h"
#include "../scanner.h"
#include "../parser.h"


using std::find;
using std::string;
using std::vector;


NodeInt *P(int value) {
    return new NodeInt(value);
}


NodePair *P(const string &key, Node *value) {
    auto ret = new NodePair(NodeString::Ptr(new NodeString(key)), Node::Ptr());
    ret->value.reset(value);
    return ret;
}


NodeList *LP(const vector<Node *> &nodes) {
    NodeList* list = new NodeList();
    for (auto child : nodes) {
        list->value.emplace_back(child);
    }
    return list;
}


NodeInt::Ptr N(int value) {
    return NodeInt::Ptr(P(value));
}


NodeList::Ptr L(const vector<Node *> &nodes) {
    return NodeList::Ptr(LP(nodes));
}


NodeObject::Ptr O(const vector<NodePair *> &pairs) {
    NodeObject::Ptr obj = NodeObject::Ptr(new NodeObject());
    for (auto pair : pairs) {
        obj->pairs.emplace_back(pair);
    }
    return obj;
}


Node::Ptr parse(const string &str) {
    Scanner scanner;
    for (char ch : str) {
        scanner.feed(ch);
    }
    scanner.feed('\0');

    Parser parser;
    Token::Ptr tok;
    while ((tok = scanner.pop())) {
        parser.feed(*tok);
    }
    REQUIRE(parser.is_finished());
    return parser.pop_result();
}


TEST_CASE("Test parser") {
    CHECK(*parse("null") == NodeNull());
    CHECK(*parse("true") == NodeBool(true));
    CHECK(*parse("false") == NodeBool(false));
    CHECK(*parse("1") == *N(1));

    CHECK(*parse("[]") == *L({}));
    CHECK(*parse("[2]") == *L({P(2)}));
    CHECK(*parse("[2, 3]") == *L({P(2), P(3)}));

    CHECK(*parse("{}") == *O({}));
    CHECK(*parse("{\"a\": 1}") == *O({P("a", P(1))}));
    CHECK(*parse("{\"a\": 1, \"b\": 2}") == *O({
        P("a", P(1)),
        P("b", P(2)),
    }));

    CHECK(*parse("{\"a\": [1]}") == *O({
        P("a", LP({P(1)})),
    }));
}


TEST_CASE("Test parser throw") {
    REQUIRE_THROWS_AS(parse("{[]: 1}"), UnexpectedToken);
    try {
        parse("{[]: 1}");
    } catch (UnexpectedToken &exc) {
        CHECK(exc.token->type == TokenType::LSQUARE);
        CHECK(exc.token->start == SourcePos(0, 1));
        CHECK(exc.expected_types == decltype(exc.expected_types){TokenType::STRING});
    }
}


TEST_CASE("Test clone_node") {
    NodeObject::Ptr node = O({
        P("a", LP({P(1)})),
        P("b", P(2)),
    });
    CHECK(*node == *clone_node<NodeObject>(*node));
}
