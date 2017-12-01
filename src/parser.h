#ifndef JSON_CXX_PARSER_H
#define JSON_CXX_PARSER_H

#include <utility>
#include <vector>

#include "node.h"
#include "scanner.h"


using std::move;
using std::vector;


class Parser {
public:
    Parser() : states({&Parser::st_json}) {}
    void feed(const Token &tok);
    Node::Ptr pop_result();
    bool is_finished() const;
    void reset();

    Parser &enable_comment(bool value) {
        this->comment = value;
        return *this;
    }

private:
    vector<void (Parser::*)(const Token &)> states;
    vector<Node::Ptr> nodes;
    bool comment = false;

    void unexpected_token(const Token &tok, const vector<TokenType> &expected);
    void enter_json();
    void enter_list();
    void enter_list_item();
    void enter_object();
    void enter_object_item();
    void enter_pair();
    void leave();

    void st_json(const Token &tok);
    void st_json_end(const Token &tok);
    void st_string(const Token &tok);
    void st_list(const Token &tok);
    void st_list_end(const Token &tok);
    void st_pair(const Token &tok);
    void st_pair_end(const Token &tok);
    void st_object(const Token &tok);
    void st_object_end(const Token &tok);

    template<class Tokenclass, class NodeClass>
    void handle_simple_token(const Token &tok) {
        NodeClass *node = new NodeClass(static_cast<const Tokenclass &>(tok).value);
        this->nodes.emplace_back(node);
        this->states.pop_back();
    }
};


#endif //JSON_CXX_PARSER_H
