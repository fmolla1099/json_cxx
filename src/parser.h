#ifndef JSON_CXX_PARSER_H
#define JSON_CXX_PARSER_H

#include <utility>
#include <vector>

#include "node.h"
#include "scanner.h"


using std::move;
using std::vector;


enum class ParserState {
    JSON,
    JSON_END,
    STRING,
    LIST,
    LIST_END,
    PAIR,
    PAIR_END,
    OBJECT,
    OBJECT_END,
};


class Parser {
public:
    Parser();
    void feed(const Token &tok);
    Node::Ptr pop_result();
    bool is_finished() const;

private:
    vector<ParserState> states;
    vector<Node::Ptr> nodes;

    void unexpected_token(const Token &tok, const vector<TokenType> &expected);
    void enter_json();
    void enter_list();
    void enter_list_item();
    void enter_object();
    void enter_object_item();
    void enter_pair();

    template<class Tokenclass, class NodeClass>
    void handle_simple_token(const Token &tok) {
        NodeClass *node = new NodeClass(reinterpret_cast<const Tokenclass &>(tok).value);
        this->nodes.emplace_back(node);
        this->states.pop_back();
    }
};


#endif //JSON_CXX_PARSER_H
