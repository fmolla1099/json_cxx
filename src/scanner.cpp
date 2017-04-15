#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <memory>
#include <string>
#include <utility>

#include "scanner.h"
#include "exceptions.h"
#include "utils.hpp"


using std::atoll;
using std::atoi;
using std::move;
using std::numeric_limits;
using std::pow;
using std::string;
using std::to_string;


string Token::name() const {
    return "Token:" + string(1, static_cast<char>(this->type));
}


string Token::repr_value() const {
    return "";
}


string Token::repr_short() const {
    string name = this->name();
    string value = this->repr_value();
    if (!value.empty()) {
        return name + " " + value;
    } else {
        return name;
    }
}


string Token::repr_full() const {
    return "<" + this->repr_short()
        + " start=" + repr(this->start) + " end=" + repr(this->end) + ">";
}


bool Token::operator==(const Token &other) const {
    return this->type == other.type;
}


bool Token::operator!=(const Token &other) const {
    return !(*this == other);
}


#define DEFINE_TOKEN_COMMON(token_cls) \
bool token_cls::operator==(const Token &other) const { \
    const token_cls *tok = dynamic_cast<const token_cls *>(&other); \
    if (tok != nullptr) { \
        return this->value == tok->value; \
    } else { \
        return false; \
    } \
} \
string token_cls::name() const { \
    return #token_cls; \
}


DEFINE_TOKEN_COMMON(TokenInt)
DEFINE_TOKEN_COMMON(TokenFloat)
DEFINE_TOKEN_COMMON(TokenString)

#undef DEFINE_TOKEN_COMMON


string TokenInt::repr_value() const {
    return to_string(this->value);
}


string TokenFloat::repr_value() const {
    return to_string(this->value);
}


string TokenString::repr_value() const {
    return this->value;
}


void Scanner::feed(char ch) {
    this->prev_pos = this->cur_pos;
    this->cur_pos.add_char(ch);
    this->refeed(ch);
}


void Scanner::refeed(char ch) {
    switch (this->state) {
        case ScannerState::INIT:
            return this->st_init(ch);
        case ScannerState::NUMBER:
            return this->st_number(ch);
        case ScannerState::STRING:
            return this->st_string(ch);
        case ScannerState::ENDED:
            return this->exception("received char in ENDED state");
    }
    assert(!"Unreachable");
}


Token::Ptr Scanner::pop() {
    if (this->buffer.empty()) {
        return Token::Ptr();
    } else {
        Token::Ptr tok = move(this->buffer.front());
        this->buffer.pop_front();
        return tok;
    }
}


void Scanner::st_init(char ch) {
    this->start_pos = this->cur_pos;
    if (ch == '\0') {
        Token *tok = new Token(TokenType::END);
        tok->start = this->cur_pos;
        tok->end = this->cur_pos;
        this->buffer.emplace_back(tok);
        this->state = ScannerState::ENDED;
    } else if (isspace(ch)) {
        // pass
    } else if (string("[]{},:").find(ch) != string::npos) {
        Token *tok = new Token(static_cast<TokenType>(ch));
        tok->start = this->cur_pos;
        tok->end = this->cur_pos;
        this->buffer.emplace_back(tok);
    } else if (ch == '"') {
        this->state = ScannerState::STRING;
        this->refeed(ch);
    } else if (isdigit(ch) || ch == '.' || ch == '+' || ch == '-') {
        this->state = ScannerState::NUMBER;
        this->refeed(ch);
    } else {
        this->unknown_char(ch);
    }
}


void Scanner::st_number(char ch) {
    NumberState &ns = this->num_state;
    if (ns.state == NumberSubState::INIT) {
        ns.state = NumberSubState::SIGNED;
        if (ch == '-') {
            ns.num_sign = -1;
        } else if (ch == '+') {
            // pass
        } else {
            this->st_number(ch);
        }
    } else if (ns.state == NumberSubState::SIGNED) {
        if (isdigit(ch)) {
            ns.int_digits.push_back(ch);
            ns.state = NumberSubState::INT_DIGIT;
        } else if (ch == '.') {
            ns.has_dot = true;
            ns.state = NumberSubState::LEADING_DOT;
        } else {
            this->unknown_char(ch, "expect dot or digit");
        }
    } else if (ns.state == NumberSubState::INT_DIGIT) {
        if (isdigit(ch)) {
            ns.int_digits.push_back(ch);
        } else if (ch == '.') {
            ns.has_dot = true;
            ns.state = NumberSubState::DOTTED;
        } else if (ch == 'e' || ch == 'E') {
            ns.state = NumberSubState::EXP;
        } else {
            this->finish_num(ch);
        }
    } else if (ns.state == NumberSubState::LEADING_DOT) {
        if (isdigit(ch)) {
            ns.dot_digits.push_back(ch);
            ns.state = NumberSubState::DOTTED;
        } else {
            this->unknown_char(ch, "expect digit");
        }
    } else if (ns.state == NumberSubState::DOTTED) {
        if (isdigit(ch)) {
            ns.dot_digits.push_back(ch);
        } else if (ch == 'e' || ch == 'E') {
            ns.state = NumberSubState::EXP;
        } else {
            this->finish_num(ch);
        }
    } else if (ns.state == NumberSubState::EXP) {
        if (ch == '+' || ch == '-') {
            ns.state = NumberSubState::EXP_SIGNED;
            if (ch == '-') {
                ns.exp_sign = -1;
            }
        } else if (isdigit(ch)) {
            ns.exp_digits.push_back(ch);
            ns.state = NumberSubState::EXP_DIGIT;
        } else {
            this->unknown_char(ch, "expect digit or sign");
        }
    } else if (ns.state == NumberSubState::EXP_SIGNED) {
        if (isdigit(ch)) {
            ns.exp_digits.push_back(ch);
            ns.state = NumberSubState::EXP_DIGIT;
        } else {
            this->unknown_char(ch, "expect digit");
        }
    } else if (ns.state == NumberSubState::EXP_DIGIT) {
        if (isdigit(ch)) {
            ns.exp_digits.push_back(ch);
        } else {
            this->finish_num(ch);
        }
    } else {
        assert(!"Unreachable");
    }
}


void Scanner::st_string(char ch) {
    StringState &ss = this->string_state;
    if (ss.state == StringSubState::INIT) {
        if (ch != '"') {
            this->unknown_char(ch, "expect double quote");
        } else {
            ss.state = StringSubState::NORMAL;
        }
    } else if (ss.state == StringSubState::NORMAL) {
        if (ch == '"') {
            Token *tok = new TokenString(ss.value);
            tok->start = this->start_pos;
            tok->end = this->cur_pos;
            this->buffer.emplace_back(tok);
            // reset
            this->string_state = StringState();
            this->state = ScannerState::INIT;
        } else if (ch == '\\') {
            ss.state = StringSubState::ESCAPE;
        } else {
            ss.value.push_back(ch);
        }
    } else if (ss.state == StringSubState::ESCAPE) {
        assert(!"Unimplemented");
    } else if (ss.state == StringSubState::HEX) {
        assert(!"Unimplemented");
    } else {
        assert(!"Unreachable");
    }
}


void Scanner::exception(const string &msg) {
    throw TokenizerError(msg, this->start_pos, this->cur_pos);
}


void Scanner::unknown_char(char ch, const string &additional) {
    string msg = "Unknown char: " + string (1, ch);
    if (!additional.empty()) {
        msg += ", " + additional;
    }
    this->exception(msg);
}


void Scanner::finish_num(char ch) {
    Token *tok = this->num_state.to_token();
    tok->start = this->start_pos;
    tok->end = this->prev_pos;
    this->buffer.emplace_back(tok);
    // reset states
    this->num_state = NumberState();
    this->state = ScannerState::INIT;
    this->refeed(ch);
}


Token *NumberState::to_token() const {
    int64_t iv = atoll(this->int_digits.data());
    double fv = iv;

    double div = 10;
    for (char ch : this->dot_digits) {
        fv += (ch - '0') / div;
        div *= 10;
    }

    if (!this->exp_digits.empty()) {
        int exp = atoi(this->exp_digits.data()) * this->exp_sign;
        fv *= pow(10, exp);
        iv *= pow(10, exp);
    }

    iv *= this->num_sign;
    fv *= this->num_sign;

    if (!this->has_dot && this->exp_sign > 0
        && numeric_limits<int64_t>::min() < fv && fv < numeric_limits<int64_t>::max())
    {
        return new TokenInt(iv);
    } else {
        return new TokenFloat(fv);
    }
}
