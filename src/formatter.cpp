#include <algorithm>
#include <cassert>
#include <string>

#include "formatter.h"
#include "string_fmt.hpp"
#include "unicode.h"


using std::all_of;
using std::string;


ostream &Formatter::format(ostream &os, const Node &node) {
    FormatContext ctx(this->opt);
    this->do_node(os, node, ctx);
    return os;
}


void Formatter::do_node(ostream &os, const Node &node, FormatContext &ctx) {
    switch (node.type) {
    case NodeType::NIL:
        return this->do_null(os, static_cast<const NodeNull &>(node), ctx);
    case NodeType::BOOL:
        return this->do_bool(os, static_cast<const NodeBool &>(node), ctx);
    case NodeType::INT:
        return this->do_int(os, static_cast<const NodeInt &>(node), ctx);
    case NodeType::FLOAT:
        return this->do_float(os, static_cast<const NodeFloat &>(node), ctx);
    case NodeType::STRING:
        return this->do_string(os, static_cast<const NodeString &>(node), ctx);
    case NodeType::LIST:
        return this->do_list(os, static_cast<const NodeList &>(node), ctx);
    case NodeType::PAIR:
        return this->do_pair(os, static_cast<const NodePair &>(node), ctx);
    case NodeType::OBJECT:
        return this->do_object(os, static_cast<const NodeObject &>(node), ctx);
    }
    assert(!"Unreachable");
}


void Formatter::do_null(ostream &os, const NodeNull &node, FormatContext &ctx) {
    this->do_indent(os, ctx);
    os << "null";
}


void Formatter::do_bool(ostream &os, const NodeBool &node, FormatContext &ctx) {
    this->do_indent(os, ctx);
    os << (node.value ? "true" : "false");
}


void Formatter::do_int(ostream &os, const NodeInt &node, FormatContext &ctx) {
    this->do_indent(os, ctx);
    os << to_string(node.value);
}


void Formatter::do_float(ostream &os, const NodeFloat &node, FormatContext &ctx) {
    this->do_indent(os, ctx);
    os << to_string(node.value);    // TODO: handle overflow, precision
}


void Formatter::do_string(ostream &os, const NodeString &node, FormatContext &ctx) {
    this->do_indent(os, ctx);
    os << "\"";
    for (unichar ch : node.value) {
        os << Formatter::quote_char(ch, ctx.opt);
    }
    os << "\"";
}


void Formatter::do_list(ostream &os, const NodeList &node, FormatContext &ctx) {
    bool simple_child = (
        node.value.size() <= 1
        || all_of(node.value.begin(), node.value.end(), Formatter::is_simple_node<Node>)
    );
    this->do_list_like<Node>(os, node.value, ctx, "[", "]", simple_child);
}


void Formatter::do_pair(ostream &os, const NodePair &node, FormatContext &ctx) {
    ctx.push();
    this->do_indent(os, ctx);

    ctx.newline = false;
    this->do_string(os, *node.key, ctx);
    os << ": ";
    this->do_node(os, *node.value, ctx);

    ctx.pop();
}


void Formatter::do_object(ostream &os, const NodeObject &node, FormatContext &ctx) {
    bool simple_child = (
        node.pairs.empty()
        || all_of(node.pairs.begin(), node.pairs.end(), Formatter::is_simple_node<NodePair>)
    );
    this->do_list_like<NodePair>(os, node.pairs, ctx, "{", "}", simple_child);
}


template<class NodeClass>
void Formatter::do_list_like(
    ostream &os, const vector<typename NodeClass::Ptr> &children, FormatContext &ctx,
    const string &open, const string &close, bool simple_child)
{
    ctx.push();

    this->do_indent(os, ctx);
    if (simple_child) {
        os << open;
        ctx.newline = false;
    } else {
        os << open << "\n";
        ctx.newline = true;
        ctx.level++;
    }

    for (size_t i = 0; i < children.size(); ++i) {
        this->do_node(os, *children[i], ctx);
        if (i != children.size() - 1) {
            if (simple_child) {
                os << ", ";
            } else {
                os << ",\n";
            }
        } else {
            if (!simple_child) {
                os << "\n";
            }
        }
    }

    if (ctx.newline) {
        ctx.level--;
        this->do_indent(os, ctx);
    }
    os << close;

    ctx.pop();
}


void Formatter::do_indent(ostream &os, FormatContext &ctx) {
    if (ctx.newline) {
        if (ctx.opt.use_tab()) {
            os << string(ctx.level, '\t');
        } else {
            os << string(ctx.level * ctx.opt.indent(), ' ');
        }
    }
}


template<class NodeClass>
bool Formatter::is_simple_node(const typename NodeClass::Ptr &node) {
    return Formatter::is_simple_node(*node);
}


bool Formatter::is_simple_node(const Node &node) {
    if (
        node.type == NodeType::NIL || node.type == NodeType::BOOL
        || node.type == NodeType::INT || node.type == NodeType::FLOAT
        || node.type == NodeType::STRING)
    {
        return true;
    } else if (node.type == NodeType::LIST) {
        const auto &list = static_cast<const NodeList &>(node);
        return Formatter::is_simple_list(list);
    } else if (node.type == NodeType::OBJECT) {
        const auto &obj = static_cast<const NodeObject &>(node);
        return obj.pairs.empty();
    } else {
        return false;
    }
}


bool Formatter::is_simple_list(const NodeList &list) {
    if (list.value.empty()) {
        return true;
    }
    if (list.value.size() == 1) {
        if (Formatter::is_simple_node(*list.value[0])) {
            return true;
        }
    }
    return false;
}


string Formatter::quote_char(unichar ch, const FormatOption &opt) {
    if (ch == '"') {
        return "\\\"";
    } else if (ch == '\\') {
        return "\\\\";
    } else if (ch == '/') {
        return "/";     // no escape
    } else if (ch == '\b') {
        return "\\b";
    } else if (ch == '\f') {
        return "\\f";
    } else if (ch == '\n') {
        return "\\n";
    } else if (ch == '\t') {
        return "\\t";
    } else if (ch < 0x20) {
        return string_fmt("\\u%04x", ch);
    } else {
        return u8_encode({ch});
    }
}
