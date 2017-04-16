#ifndef JSON_CXX_PARSER_H
#define JSON_CXX_PARSER_H

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "scanner.h"


using std::move;
using std::string;
using std::unique_ptr;
using std::vector;


enum class NodeType {
    INT,
    FLOAT,
    STRING,
    LIST,
    PAIR,
    OBJECT,
};


struct Node {
    typedef unique_ptr<Node> Ptr;

    Node(NodeType type) : type(type) {}
    virtual Node *clone() const = 0;
    virtual bool operator==(const Node &other) const = 0;
    virtual bool operator!=(const Node &other) const;
    virtual string repr(unsigned int indent = 0) const = 0;

    NodeType type;
};


REPR(Node) {
    return value.repr();
}


#define NODE_COMMON_DECL(node_type) \
typedef unique_ptr<node_type> Ptr; \
virtual bool operator==(const Node &other) const; \
virtual string repr(unsigned int indent = 0) const; \
virtual node_type *clone() const


struct NodeInt : Node {
    NodeInt(int64_t value) : Node(NodeType::INT), value(value) {}
    int64_t value;

    NODE_COMMON_DECL(NodeInt);
};


struct NodeFloat : Node {
    NodeFloat(double value) : Node(NodeType::FLOAT), value(value) {}
    double value;

    NODE_COMMON_DECL(NodeFloat);
};


struct NodeString : Node {
    NodeString(const string &value) : Node(NodeType::STRING), value(value) {}
    string value;

    NODE_COMMON_DECL(NodeString);
};


struct NodeList : Node {
    NodeList() : Node(NodeType::LIST) {}
    vector<Node::Ptr> value;

    NODE_COMMON_DECL(NodeList);
};


struct NodePair : Node {
    NodePair(NodeString::Ptr &&key, Node::Ptr &&value)
        : Node(NodeType::PAIR) , key(move(key)), value(move(value))
    {}
    NodeString::Ptr key;
    Node::Ptr value;

    NODE_COMMON_DECL(NodePair);
};


struct NodeObject : Node {
    NodeObject() : Node(NodeType::OBJECT) {}
    vector<NodePair::Ptr> pairs;

    NODE_COMMON_DECL(NodeObject);
};


#undef NODE_COMMON_DECL


template<class NodeClass>
typename NodeClass::Ptr clone_node(const Node &node) {
    NodeClass *cloned = dynamic_cast<NodeClass *>(node.clone());
    return typename NodeClass::Ptr(cloned);
}


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
};


#endif //JSON_CXX_PARSER_H
