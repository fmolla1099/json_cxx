#include <cassert>
#include <string>

#include "exceptions.h"
#include "parser.h"
#include "utils.hpp"


using std::to_string;


bool Node::operator!=(const Node &other) const {
    return !(*this == other);
}


bool NodeNull::operator==(const Node &other) const {
    return this->type == other.type;
}


string NodeNull::repr(unsigned int indent) const {
    return string(indent * 4, ' ') + "null";
}


NodeNull *NodeNull::clone() const {
    return new NodeNull();
}


#define SIMPLE_NODE_COMMON(node_type) \
bool node_type::operator==(const Node &other) const { \
    const node_type *node = dynamic_cast<const node_type *>(&other); \
    if (node != nullptr) { \
        return this->value == node->value; \
    } else { \
        return false; \
    } \
} \
string node_type::repr(unsigned int indent) const { \
    return string(indent * 4, ' ') + ::repr(this->value); \
} \
node_type *node_type::clone() const { \
    return new node_type(this->value); \
}


SIMPLE_NODE_COMMON(NodeBool);
SIMPLE_NODE_COMMON(NodeInt);
SIMPLE_NODE_COMMON(NodeFloat);
SIMPLE_NODE_COMMON(NodeString);


bool NodeList::operator==(const Node &other) const {
    const NodeList *node = dynamic_cast<const NodeList *>(&other);
    if (node == nullptr) {
        return false;
    }
    if (this->value.size() != node->value.size()) {
        return false;
    }
    for (size_t i = 0; i < this->value.size(); ++i) {
        if (*this->value[i] != *node->value[i]) {
            return false;
        }
    }
    return true;
}


string NodeList::repr(unsigned int indent) const {
    string ans;
    ans += string(indent * 4, ' ') + "[\n";
    for (size_t i = 0; i < this->value.size(); ++i) {
        ans += this->value[i]->repr(indent + 1);
        if (i != this->value.size() - 1) {
            ans += ",\n";
        } else {
            ans += "\n";
        }
    }
    ans += string(indent * 4, ' ') + "]";
    return ans;
}


NodeList *NodeList::clone() const {
    NodeList *list = new NodeList();
    for (const Node::Ptr &child : this->value) {
        list->value.emplace_back(child->clone());
    }
    return list;
}


bool NodePair::operator==(const Node &other) const {
    const NodePair *node = dynamic_cast<const NodePair *>(&other);
    if (node == nullptr) {
        return false;
    }
    return *this->key == *node->key && *this->value == *node->value;
}


string NodePair::repr(unsigned int indent) const {
    return string(indent * 4, ' ') + this->key->repr() + ":\n" + this->value->repr(indent + 1);
}


NodePair *NodePair::clone() const {
    return new NodePair(
        NodeString::Ptr(this->key->clone()),
        Node::Ptr(this->value->clone())
    );
}


bool NodeObject::operator==(const Node &other) const {
    const NodeObject *node = dynamic_cast<const NodeObject *>(&other);
    if (node == nullptr) {
        return false;
    }
    if (this->pairs.size() != node->pairs.size()) {
        return false;
    }
    for (size_t i = 0; i < this->pairs.size(); ++i) {
        if (*this->pairs[i] != *node->pairs[i]) {
            return false;
        }
    }
    return true;
}


string NodeObject::repr(unsigned int indent) const {
    string ans;
    ans += string(indent * 4, ' ') + "{\n";
    for (size_t i = 0; i < this->pairs.size(); ++i) {
        ans += this->pairs[i]->repr(indent + 1);
        if (i != this->pairs.size() - 1) {
            ans += ",\n";
        } else {
            ans += "\n";
        }
    }
    ans += string(indent * 4, ' ') + "}";
    return ans;
}


NodeObject *NodeObject::clone() const {
    NodeObject *obj = new NodeObject();
    for (const NodePair::Ptr &pair : this->pairs) {
        obj->pairs.emplace_back(pair->clone());
    }
    return obj;
}


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

#define HANDLE_SIMPLE_TOKEN(token_cls, node_cls) \
        node_cls *node = new node_cls(reinterpret_cast<const token_cls &>(tok).value); \
        this->nodes.emplace_back(node); \
        this->states.pop_back()

        if (tok.type == TokenType::LSQUARE) {
            this->enter_list();
        } else if (tok.type == TokenType::LCURLY) {
            this->enter_object();
        } else if (tok.type == TokenType::NIL) {
            this->nodes.emplace_back(new NodeNull());
            this->states.pop_back();
        } else if (tok.type == TokenType::BOOL) {
            HANDLE_SIMPLE_TOKEN(TokenBool, NodeBool);
        } else if (tok.type == TokenType::INT) {
            HANDLE_SIMPLE_TOKEN(TokenInt, NodeInt);
        } else if (tok.type == TokenType::FLOAT) {
            HANDLE_SIMPLE_TOKEN(TokenFloat, NodeFloat);
        } else if (tok.type == TokenType::STRING) {
            HANDLE_SIMPLE_TOKEN(TokenString, NodeString);
        } else {
            this->unexpected_token(tok, {
                TokenType::LSQUARE, TokenType::LCURLY,
                TokenType::INT, TokenType::FLOAT, TokenType::STRING,
            });
        }

#undef HANDLE_SIMPLE_TOKEN
    } else if (this->states.back() == ParserState::JSON_END) {
        this->states.pop_back();
        this->feed(tok);
    } else if (this->states.back() == ParserState::STRING) {
        if (tok.type == TokenType::STRING) {
            NodeString *node = new NodeString(reinterpret_cast<const TokenString&>(tok).value);
            this->nodes.emplace_back(node);
            this->states.pop_back();
        } else {
            this->unexpected_token(tok, {TokenType::STRING});
        }
    } else if (this->states.back() == ParserState::LIST) {
        NodeList *node = new NodeList();
        this->nodes.emplace_back(node);
        if (tok.type == TokenType::RSQUARE) {
            this->states.pop_back();
        } else {
            this->enter_list_item();
            this->feed(tok);
        }
    } else if (this->states.back() == ParserState::LIST_END) {
        Node::Ptr node = move(this->nodes.back());
        this->nodes.pop_back();
        NodeList &list = reinterpret_cast<NodeList &>(*this->nodes.back());
        list.value.push_back(move(node));

        if (tok.type == TokenType::RSQUARE) {
            this->states.pop_back();
        } else if (tok.type == TokenType::COMMA) {
            this->enter_list_item();
        } else {
            this->unexpected_token(tok, {TokenType::RSQUARE, TokenType::COMMA});
        }
    } else if (this->states.back() == ParserState::OBJECT) {
        NodeObject *node = new NodeObject();
        this->nodes.emplace_back(node);
        if (tok.type == TokenType::RCURLY) {
            this->states.pop_back();
        } else {
            this->enter_object_item();
            this->feed(tok);
        }
    } else if (this->states.back() == ParserState::OBJECT_END) {
        if (tok.type == TokenType::RCURLY) {
            this->states.pop_back();
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

        NodeObject &obj = reinterpret_cast<NodeObject &>(*this->nodes.back());
        NodePair *pair = new NodePair(NodeString::Ptr(key), Node::Ptr(value));
        obj.pairs.emplace_back(pair);

        this->states.pop_back();
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


void Parser::unexpected_token(const Token &tok, const vector<TokenType> &expected) {
    throw UnexpectedToken(tok, expected);
}
