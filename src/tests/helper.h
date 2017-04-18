#ifndef JSON_CXX_TESTS_HELPER_H
#define JSON_CXX_TESTS_HELPER_H


#include <vector>

#include "../unicode.h"
#include "../scanner.h"
#include "../parser.h"
#include "../formatter.h"


using std::vector;


vector<Token::Ptr> get_tokens(const ustring &str);
Node::Ptr parse_string(const string &input);
string format_node(const Node &node, const FormatOption &opt = FormatOption());


#endif //JSON_CXX_TESTS_HELPER_H
