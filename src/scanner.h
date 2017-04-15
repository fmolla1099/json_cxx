#ifndef JSON_CXX_SCANNER_H
#define JSON_CXX_SCANNER_H


#include <cstdint>
#include <deque>
#include <memory>
#include <string>

#include "sourcepos.h"


using std::deque;
using std::string;
using std::unique_ptr;


enum class TokenType : char {
    INT         = 'i',
    FLOAT       = 'f',
    STRING      = 's',
    LBRACKET    = '[',
    RBRACKET    = ']',
    LBRACE      = '{',
    RBRACE      = '}',
    COMMA       = ',',
    COLON       = ':',
    COMMENT     = 'c',
    END         = '$',
};


class Token {
public:
    typedef unique_ptr<Token> Ptr;

    Token(TokenType type) : type(type) {}
    virtual string name() const;
    virtual string repr_value() const;
    virtual string repr_short() const;
    virtual string repr_full() const;
    virtual ~Token() {};
    virtual bool operator==(const Token &other) const;
    virtual bool operator!=(const Token &other) const;

    TokenType type;
    SourcePos start;
    SourcePos end;
};


REPR(Token) {
    return value.repr_full();
}


class TokenInt : public Token {
public:
    TokenInt(int64_t value) : Token(TokenType::INT), value(value) {}
    virtual string name() const;
    virtual string repr_value() const;
    virtual bool operator==(const Token &other) const;

    int64_t value;
};


class TokenFloat : public Token {
public:
    TokenFloat(double value) : Token(TokenType::FLOAT), value(value) {}
    virtual string name() const;
    virtual string repr_value() const;
    virtual bool operator==(const Token &other) const;

    double value;
};


class TokenString : public Token {
public:
    TokenString(const string &value) : Token(TokenType::STRING), value(value) {}
    virtual string name() const;
    virtual string repr_value() const;
    virtual bool operator==(const Token &other) const;

    string value;
};


enum class ScannerState {
    INIT,
    NUMBER,
    STRING,
    ENDED,
};


enum class NumberSubState {
    INIT,
    SIGNED,
    INT_DIGIT,
    DOTTED,
    LEADING_DOT,
    EXP,
    EXP_SIGNED,
    EXP_DIGIT,
};


struct NumberState {
    NumberSubState state = NumberSubState::INIT;

    string int_digits;
    string dot_digits;
    string exp_digits;
    int num_sign = 1;
    int exp_sign = 1;
    bool has_dot = false;

    Token *to_token() const;
};


enum class StringSubState {
    INIT,
    NORMAL,
    ESCAPE,
    HEX,
};


struct StringState {
    StringSubState state = StringSubState::INIT;
    string value;
};


class Scanner {
public:
    void feed(char ch);
    Token::Ptr pop();

private:
    void refeed(char ch);
    void st_init(char ch);
    void st_number(char ch);
    void st_string(char ch);
    void finish_num(char ch);
    void exception(const string &msg);
    void unknown_char(char ch, const string &additional = "");

    ScannerState state = ScannerState::INIT;
    deque<Token::Ptr> buffer;
    SourcePos start_pos;
    SourcePos prev_pos;
    SourcePos cur_pos;

    NumberState num_state;
    StringState string_state;
};


#endif //JSON_CXX_SCANNER_H
