#include "node.h"


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
