#include "exceptions.h"


string UnexpectedToken::make_msg(const Token &token, const vector<TokenType> &expected_types) {
    string types;
    for (TokenType tt : expected_types) {
        types.push_back(static_cast<char>(tt));
    }
    return "Unexpected token: " + repr(token)  + ", expected types: '" + types + "'.";
}
