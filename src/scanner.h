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
    NIL     = 'n',
    BOOL    = 'b',
    INT     = 'i',
    FLOAT   = 'f',
    STRING  = 's',
    LSQUARE = '[',
    RSQUARE = ']',
    LCURLY  = '{',
    RCURLY  = '}',
    COMMA   = ',',
    COLON   = ':',
    COMMENT = 'c',
    END     = '$',
};


class Token {
public:
    typedef unique_ptr<Token> Ptr;

    explicit Token(TokenType type) : type(type) {}
    virtual Token *clone() const;
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


template<class ValueType, TokenType tok_type>
class ExtendedToken : public Token {
public:
    typedef ExtendedToken<ValueType, tok_type> _SelfType;
    typedef unique_ptr<_SelfType> Ptr;

    explicit ExtendedToken(const ValueType &value) : Token(tok_type), value(value) {}

    virtual string name() const;
    virtual string repr_value() const;

    virtual ExtendedToken<ValueType, tok_type> *clone() const {
        _SelfType *tok = new _SelfType(this->value);
        tok->start = this->start;
        tok->end = this->end;
        return tok;
    };

    virtual bool operator==(const Token &other) const {
        auto tok = dynamic_cast<const _SelfType *>(&other);
        if (tok != nullptr) {
            return this->value == tok->value;
        } else {
            return false;
        }
    }

    ValueType value;
};


typedef ExtendedToken<bool, TokenType::BOOL> TokenBool;
typedef ExtendedToken<int64_t, TokenType::INT> TokenInt;
typedef ExtendedToken<double, TokenType::FLOAT> TokenFloat;
typedef ExtendedToken<string, TokenType::STRING> TokenString;


enum class ScannerState {
    INIT,
    ID,
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


struct IdState {
    string value;
};


class Scanner {
public:
    void feed(char ch);
    Token::Ptr pop();

private:
    void refeed(char ch);
    void st_init(char ch);
    void st_id(char ch);
    void st_number(char ch);
    void st_string(char ch);
    void finish_num(char ch);
    void exception(
        const string &msg,
        SourcePos start = SourcePos(), SourcePos end = SourcePos()
    );
    void unknown_char(char ch, const string &additional = "");

    ScannerState state = ScannerState::INIT;
    deque<Token::Ptr> buffer;
    SourcePos start_pos;
    SourcePos prev_pos;
    SourcePos cur_pos;

    NumberState num_state;
    StringState string_state;
    IdState id_state;
};


#endif //JSON_CXX_SCANNER_H
