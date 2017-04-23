#include <cassert>
#include <string>
#include <utility>

#include "exceptions.h"
#include "parser.h"


using std::move;
using std::to_string;


Parser::Parser() {
    this->states.push_back(ParserState::JSON);
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


void Parser::feed(const Token &tok) {
    if (this->states.empty()) {
        if (tok.type != TokenType::END) {
            this->unexpected_token(tok, {TokenType::END});
        }
    } else if (this->states.back() == ParserState::JSON) {
        this->states.back() = ParserState::JSON_END;

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
    } else if (this->states.back() == ParserState::JSON_END) {
        this->leave();
        this->feed(tok);
    } else if (this->states.back() == ParserState::STRING) {
        if (tok.type == TokenType::STRING) {
            NodeString *node = new NodeString(static_cast<const TokenString&>(tok).value);
            this->nodes.emplace_back(node);
            this->leave();
        } else {
            this->unexpected_token(tok, {TokenType::STRING});
        }
    } else if (this->states.back() == ParserState::LIST) {
        NodeList *node = new NodeList();
        this->nodes.emplace_back(node);
        if (tok.type == TokenType::RSQUARE) {
            this->leave();
        } else {
            this->enter_list_item();
            this->feed(tok);
        }
    } else if (this->states.back() == ParserState::LIST_END) {
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
    } else if (this->states.back() == ParserState::OBJECT) {
        NodeObject *node = new NodeObject();
        this->nodes.emplace_back(node);
        if (tok.type == TokenType::RCURLY) {
            this->leave();
        } else {
            this->enter_object_item();
            this->feed(tok);
        }
    } else if (this->states.back() == ParserState::OBJECT_END) {
        if (tok.type == TokenType::RCURLY) {
            this->leave();
        } else if (tok.type == TokenType::COMMA) {
            this->enter_object_item();
        } else {
            this->unexpected_token(tok, {TokenType::RCURLY, TokenType::COMMA});
        }
    } else if (this->states.back() == ParserState::PAIR) {
        if (tok.type == TokenType::COLON) {
            this->states.back() = ParserState::PAIR_END;
            this->enter_json();
        } else {
            this->unexpected_token(tok, {TokenType::COLON});
        }
    } else if (this->states.back() == ParserState::PAIR_END) {
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
    } else {
        assert(!"Unreachable");
    }
}


void Parser::enter_json() {
    this->states.push_back(ParserState::JSON);
}


void Parser::enter_list() {
    this->states.push_back(ParserState::LIST);
}


void Parser::enter_list_item() {
    this->states.back() = ParserState::LIST_END;
    this->enter_json();
}


void Parser::enter_object() {
    this->states.push_back(ParserState::OBJECT);
}


void Parser::enter_object_item() {
    this->states.back() = ParserState::OBJECT_END;
    this->enter_pair();
}


void Parser::enter_pair() {
    this->states.push_back(ParserState::PAIR);
    this->states.push_back(ParserState::STRING);
}


void Parser::leave() {
    do {
        this->states.pop_back();
    } while (!this->states.empty() && this->states.back() == ParserState::JSON_END);
}


void Parser::unexpected_token(const Token &tok, const vector<TokenType> &expected) {
    throw UnexpectedToken(tok, expected);
}
