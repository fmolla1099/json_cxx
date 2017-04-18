#include <sstream>

#include "node.h"
#include "formatter.h"


using std::ostringstream;


bool Node::operator!=(const Node &other) const {
    return !(*this == other);
}


string Node::repr() const {
    ostringstream os;
    g_default_formatter.format(os, *this);
    return os.str();
}


bool NodeNull::operator==(const Node &other) const {
    return this->type == other.type;
}


NodeNull *NodeNull::clone() const {
    return new NodeNull();
}


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


NodeObject *NodeObject::clone() const {
    NodeObject *obj = new NodeObject();
    for (const NodePair::Ptr &pair : this->pairs) {
        obj->pairs.emplace_back(pair->clone());
    }
    return obj;
}
