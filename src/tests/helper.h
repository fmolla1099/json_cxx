#ifndef JSON_CXX_TESTS_HELPER_H
#define JSON_CXX_TESTS_HELPER_H


#include <vector>

#include "../scanner.h"
#include "../parser.h"
#include "../unicode.h"


using std::vector;


vector<Token::Ptr> get_tokens(const ustring &str);
Node::Ptr parse_string(const string &input);


#endif //JSON_CXX_TESTS_HELPER_H
