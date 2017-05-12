#include <cassert>
#include <string>
#include <utility>

#include "exceptions.h"
#include "parser.h"


using std::move;
using std::to_string;


Parser::Parser() {
    this->states.push_back(&Parser::st_json);
}


Node::Ptr Parser::pop_result() {
    assert(this->is_finished());
    Node::Ptr node = move(this->nodes.back());
    this->nodes.pop_back();
    return node;
}


bool Parser::is_finished() const {
    return this->nodes.size() == 1 && this->states.empty();
}


void Parser::reset() {
    *this = Parser();
}


void Parser::feed(const Token &tok) {
    if (this->states.empty()) {
        if (tok.type != TokenType::END) {
            this->unexpected_token(tok, {TokenType::END});
        }
    } else {
        (this->*this->states.back())(tok);
    }
}


void Parser::enter_json() {
    this->states.push_back(&Parser::st_json);
}


void Parser::enter_list() {
    this->states.push_back(&Parser::st_list);
}


void Parser::enter_list_item() {
    this->states.back() = &Parser::st_list_end;
    this->enter_json();
}


void Parser::enter_object() {
    this->states.push_back(&Parser::st_object);
}


void Parser::enter_object_item() {
    this->states.back() = &Parser::st_object_end;
    this->enter_pair();
}


void Parser::enter_pair() {
    this->states.push_back(&Parser::st_pair);
    this->states.push_back(&Parser::st_string);
}


void Parser::leave() {
    do {
        this->states.pop_back();
    } while (!this->states.empty() && this->states.back() == &Parser::st_json_end);
}


void Parser::unexpected_token(const Token &tok, const vector<TokenType> &expected) {
    throw UnexpectedToken(tok, expected);
}


void Parser::st_json(const Token &tok) {
    this->states.back() = &Parser::st_json_end;

    if (tok.type == TokenType::LSQUARE) {
        this->enter_list();
    } else if (tok.type == TokenType::LCURLY) {
        this->enter_object();
    } else if (tok.type == TokenType::NIL) {
        this->nodes.emplace_back(new NodeNull());
        this->leave();
    } else if (tok.type == TokenType::BOOL) {
        this->handle_simple_token<TokenBool, NodeBool>(tok);
    } else if (tok.type == TokenType::INT) {
        this->handle_simple_token<TokenInt, NodeInt>(tok);
    } else if (tok.type == TokenType::FLOAT) {
        this->handle_simple_token<TokenFloat, NodeFloat>(tok);
    } else if (tok.type == TokenType::STRING) {
        this->handle_simple_token<TokenString, NodeString>(tok);
    } else {
        this->unexpected_token(tok, {
            TokenType::LSQUARE, TokenType::LCURLY,
            TokenType::INT, TokenType::FLOAT, TokenType::STRING,
        });
    }
}


void Parser::st_json_end(const Token &tok) {
    this->leave();
    this->feed(tok);
}


void Parser::st_string(const Token &tok) {
    if (tok.type == TokenType::STRING) {
        NodeString *node = new NodeString(static_cast<const TokenString&>(tok).value);
        this->nodes.emplace_back(node);
        this->leave();
    } else {
        this->unexpected_token(tok, {TokenType::STRING});
    }
}


void Parser::st_list(const Token &tok) {
    NodeList *node = new NodeList();
    this->nodes.emplace_back(node);
    if (tok.type == TokenType::RSQUARE) {
        this->leave();
    } else {
        this->enter_list_item();
        this->feed(tok);
    }
}


void Parser::st_list_end(const Token &tok) {
    Node::Ptr node = move(this->nodes.back());
    this->nodes.pop_back();
    NodeList &list = static_cast<NodeList &>(*this->nodes.back());
    list.value.push_back(move(node));

    if (tok.type == TokenType::RSQUARE) {
        this->leave();
    } else if (tok.type == TokenType::COMMA) {
        this->enter_list_item();
    } else {
        this->unexpected_token(tok, {TokenType::RSQUARE, TokenType::COMMA});
    }
}


void Parser::st_object(const Token &tok) {
    NodeObject *node = new NodeObject();
    this->nodes.emplace_back(node);
    if (tok.type == TokenType::RCURLY) {
        this->leave();
    } else {
        this->enter_object_item();
        this->feed(tok);
    }
}


void Parser::st_object_end(const Token &tok) {
    if (tok.type == TokenType::RCURLY) {
        this->leave();
    } else if (tok.type == TokenType::COMMA) {
        this->enter_object_item();
    } else {
        this->unexpected_token(tok, {TokenType::RCURLY, TokenType::COMMA});
    }
}


void Parser::st_pair(const Token &tok) {
    if (tok.type == TokenType::COLON) {
        this->states.back() = &Parser::st_pair_end;
        this->enter_json();
    } else {
        this->unexpected_token(tok, {TokenType::COLON});
    }
}


void Parser::st_pair_end(const Token &tok) {
    Node *value = this->nodes.back().release();
    this->nodes.pop_back();
    NodeString *key = static_cast<NodeString *>(this->nodes.back().release());
    this->nodes.pop_back();
    assert(key->type == NodeType::STRING);

    NodeObject &obj = static_cast<NodeObject &>(*this->nodes.back());
    NodePair *pair = new NodePair(NodeString::Ptr(key), Node::Ptr(value));
    obj.pairs.emplace_back(pair);

    this->leave();
    this->feed(tok);
}
