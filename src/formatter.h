#ifndef JSON_CXX_FORMATTER_H
#define JSON_CXX_FORMATTER_H


#include <cassert>
#include <ostream>
#include <vector>

#include "node.h"
#include "unicode.h"


using std::ostream;
using std::vector;


#define DEFINE_OPTION(Class, Type, name, default_value) \
public: \
    const Type &name() const { return _ ## name; } \
    Class &name(const Type &value) { \
        this->_ ## name = value; \
        return *this; \
    } \
private: \
    Type _ ## name = default_value


#define DEFINE_FMT_OPT(Type, name, default_value) \
    DEFINE_OPTION(FormatOption, Type, name, default_value)


class FormatOption {
    DEFINE_FMT_OPT(unsigned int, indent, 4);
    DEFINE_FMT_OPT(bool, use_tab, false);
    // DEFINE_FMT_OPT(bool, always_newline, false);
};


struct FormatContextData {
    unsigned int level = 0;
    bool newline = false;
};


class FormatContext : public FormatContextData {
public:
    FormatContext(const FormatOption &opt) : opt(opt) {}

    void push() {
        this->stack.push_back(*this);
    }

    void pop() {
        assert(!this->stack.empty());
        static_cast<FormatContextData &>(*this) = this->stack.back();
        this->stack.pop_back();
    }

    const FormatOption &opt;

private:
    vector<FormatContextData> stack;
};


class Formatter {
public:
    Formatter(const FormatOption &opt) : opt(opt) {}
    ostream &format(ostream &os, const Node &node);

protected:
    void do_node(ostream &os, const Node &node, FormatContext &ctx);
    void do_null(ostream &os, const NodeNull &node, FormatContext &ctx);
    void do_bool(ostream &os, const NodeBool &node, FormatContext &ctx);
    void do_int(ostream &os, const NodeInt &node, FormatContext &ctx);
    void do_float(ostream &os, const NodeFloat &node, FormatContext &ctx);
    void do_string(ostream &os, const NodeString &node, FormatContext &ctx);
    void do_list(ostream &os, const NodeList &node, FormatContext &ctx);
    void do_pair(ostream &os, const NodePair &node, FormatContext &ctx);
    void do_object(ostream &os, const NodeObject &node, FormatContext &ctx);

    template<class NodeClass>
    void do_list_like(
        ostream &os, const vector<typename NodeClass::Ptr> &children, FormatContext &ctx,
        const string &open, const string &close, bool simple_child
    );
    void do_indent(ostream &os, FormatContext &ctx);

    template<class NodeClass>
    static bool is_simple_node(const typename NodeClass::Ptr &node);
    static bool is_simple_node(const Node &node);
    static bool is_simple_list(const NodeList &list);
    static string quote_char(unichar ch, const FormatOption &opt);

private:
    FormatOption opt;
};


#endif //JSON_CXX_FORMATTER_H
