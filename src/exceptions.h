#ifndef JSON_CXX_EXCEPTIONS_H
#define JSON_CXX_EXCEPTIONS_H


#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "sourcepos.h"
#include "scanner.h"


using std::shared_ptr;
using std::runtime_error;
using std::string;
using std::vector;


class BaseException : public runtime_error {
public:
    BaseException(
        const string &msg, const SourcePos &start = SourcePos(), const SourcePos &end = SourcePos()
    )
        : runtime_error(msg), start(start), end(end)
    {}

    SourcePos start;
    SourcePos end;
};


class TokenizerError : public BaseException {
public:
    TokenizerError(
        const string &msg, const SourcePos &start = SourcePos(), const SourcePos &end = SourcePos()
    )
        : BaseException(msg, start, end)
    {}
};


class ParserError : public BaseException {
public:
    ParserError(const string &msg)
        : BaseException(msg, SourcePos(), SourcePos())
    {}
};


class UnexpectedToken : public ParserError {
public:
    UnexpectedToken(const Token &token, const vector<TokenType> &expected_types)
        : ParserError(make_msg(token, expected_types)),
          token(token.clone()), expected_types(expected_types)
    {}

    // can not use unique_ptr here. unique_ptr will make this not copyable.
    shared_ptr<Token> token;
    vector<TokenType> expected_types;

private:
    static string make_msg(const Token &token, const vector<TokenType> &expected_types);
};


#endif //JSON_CXX_EXCEPTIONS_H
