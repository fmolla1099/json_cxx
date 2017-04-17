#ifndef JSON_CXX_SCANNER_H
#define JSON_CXX_SCANNER_H


#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <string>

#include "sourcepos.h"
#include "unicode.h"


using std::deque;
using std::map;
using std::string;
using std::unique_ptr;


struct CharConf {
    typedef unichar CharType;
    typedef ustring StringType;
};


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
typedef ExtendedToken<CharConf::StringType, TokenType::STRING> TokenString;


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
    ZEROED,
    INT_DIGIT,
    DOTTED,
    DOT_DIGIT,
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

    Token *to_token() const;
};


enum class StringSubState {
    INIT,
    NORMAL,
    ESCAPE,
    HEX,
    SURROGATED,
};


struct StringState {
    StringSubState state = StringSubState::INIT;
    CharConf::StringType value;
    string hex;
    bool last_surrogate = false;
};


struct IdState {
    CharConf::StringType value;
};


class Scanner {
public:
    void feed(CharConf::CharType ch);
    Token::Ptr pop();

private:
    void refeed(CharConf::CharType ch);
    void st_init(CharConf::CharType ch);
    void st_id(CharConf::CharType ch);
    void st_number(CharConf::CharType ch);
    void st_string(CharConf::CharType ch);
    void finish_num(CharConf::CharType ch);
    void exception(
        const string &msg,
        SourcePos start = SourcePos(), SourcePos end = SourcePos()
    );
    void unknown_char(CharConf::CharType ch, const string &additional = "");

    ScannerState state = ScannerState::INIT;
    deque<Token::Ptr> buffer;
    SourcePos start_pos;
    SourcePos prev_pos;
    SourcePos cur_pos;

    NumberState num_state;
    StringState string_state;
    IdState id_state;

    static const map<CharConf::CharType, CharConf::CharType> escape_map;
};


#endif //JSON_CXX_SCANNER_H
