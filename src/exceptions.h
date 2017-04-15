#ifndef JSON_CXX_EXCEPTIONS_H
#define JSON_CXX_EXCEPTIONS_H


#include <stdexcept>
#include <string>

#include "sourcepos.h"


using std::runtime_error;
using std::string;


class BaseException : public runtime_error {
public:
    BaseException(const string &msg) : runtime_error(msg) {}
};


class TokenizerError : public BaseException {
public:
    TokenizerError(
        const string &msg, const SourcePos &start = SourcePos(), const SourcePos &end = SourcePos()
    )
        : BaseException(msg), start(start), end(end)
    {}

    SourcePos start;
    SourcePos end;
};


#endif //JSON_CXX_EXCEPTIONS_H
