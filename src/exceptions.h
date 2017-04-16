#ifndef JSON_CXX_EXCEPTIONS_H
#define JSON_CXX_EXCEPTIONS_H


#include <stdexcept>
#include <string>

#include "sourcepos.h"


using std::runtime_error;
using std::string;


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
    ParserError(
        const string &msg, const SourcePos &start = SourcePos(), const SourcePos &end = SourcePos()
    )
        : BaseException(msg, start, end)
    {}
};


#endif //JSON_CXX_EXCEPTIONS_H
