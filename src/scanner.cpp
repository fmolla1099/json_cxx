#include <cassert>
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


Token *Token::clone() const {
    Token *tok = new Token(this->type);
    tok->start = this->start;
    tok->end = this->end;
    return tok;
}


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


template<>
string TokenBool::name() const {
    return "Bool";
}


template<>
string TokenBool::repr_value() const {
    return this->value ? "true" : "false";
}


template<>
string TokenInt::name() const {
    return "Int";
}


template<>
string TokenInt::repr_value() const {
    return to_string(this->value);
}


template<>
string TokenFloat::name() const {
    return "Float";
}


template<>
string TokenFloat::repr_value() const {
    return to_string(this->value);
}


template<>
string TokenString::name() const {
    return "Str";
}


template<>
string TokenString::repr_value() const {
    return u8_encode(this->value);  // TODO: escape
}


void Scanner::feed(CharConf::CharType ch) {
    this->prev_pos = this->cur_pos;
    this->cur_pos.add_char(ch);
    this->refeed(ch);
}


void Scanner::refeed(CharConf::CharType ch) {
    switch (this->state) {
        case ScannerState::INIT:
            return this->st_init(ch);
        case ScannerState::ID:
            return this->st_id(ch);
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


void Scanner::st_init(CharConf::CharType ch) {
    this->start_pos = this->cur_pos;
    if (ch == '\0') {
        Token *tok = new Token(TokenType::END);
        tok->start = this->cur_pos;
        tok->end = this->cur_pos;
        this->buffer.emplace_back(tok);
        this->state = ScannerState::ENDED;
    } else if (is_space(ch)) {
        // pass
    } else if (USTRING("[]{},:").find(ch) != ustring::npos) {
        Token *tok = new Token(static_cast<TokenType>(ch));
        tok->start = this->cur_pos;
        tok->end = this->cur_pos;
        this->buffer.emplace_back(tok);
    } else if (ch == '"') {
        this->state = ScannerState::STRING;
        this->refeed(ch);
    } else if (is_digit(ch) || ch == '.' || ch == '+' || ch == '-') {
        this->state = ScannerState::NUMBER;
        this->refeed(ch);
    } else if (is_alpha(ch)) {
        this->state = ScannerState::ID;
        this->refeed(ch);
    } else {
        this->unknown_char(ch);
    }
}


void Scanner::st_id(CharConf::CharType ch) {
    if (is_alpha(ch)) {
        this->id_state.value.push_back(ch);
    } else {
        Token *tok = nullptr;
        if (this->id_state.value == USTRING("null")) {
            tok = new Token(TokenType::NIL);
        } else if (this->id_state.value == USTRING("true")) {
            tok = new TokenBool(true);
        } else if (this->id_state.value == USTRING("false")) {
            tok = new TokenBool(false);
        } else {
            assert(!this->id_state.value.empty());
        }

        if (tok != nullptr) {
            tok->start = this->start_pos;
            tok->end = this->prev_pos;
            this->buffer.emplace_back(tok);
            // reset
            this->id_state = IdState();
            this->state = ScannerState::INIT;
            this->refeed(ch);
        } else {
            this->exception(
                "bad identifier: '" + u8_encode(this->id_state.value) + "', "
                "expect null|true|false",
                this->start_pos, this->prev_pos
            );
        }
    }
}


void Scanner::st_number(CharConf::CharType ch) {
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
        if (is_digit(ch)) {
            ns.int_digits.push_back(static_cast<char>(ch));
            ns.state = NumberSubState::INT_DIGIT;
        } else if (ch == '.') {
            ns.has_dot = true;
            ns.state = NumberSubState::LEADING_DOT;
        } else {
            this->unknown_char(ch, "expect dot or digit");
        }
    } else if (ns.state == NumberSubState::INT_DIGIT) {
        if (is_digit(ch)) {
            ns.int_digits.push_back(static_cast<char>(ch));
        } else if (ch == '.') {
            ns.has_dot = true;
            ns.state = NumberSubState::DOTTED;
        } else if (ch == 'e' || ch == 'E') {
            ns.state = NumberSubState::EXP;
        } else {
            this->finish_num(ch);
        }
    } else if (ns.state == NumberSubState::LEADING_DOT) {
        if (is_digit(ch)) {
            ns.dot_digits.push_back(static_cast<char>(ch));
            ns.state = NumberSubState::DOTTED;
        } else {
            this->unknown_char(ch, "expect digit");
        }
    } else if (ns.state == NumberSubState::DOTTED) {
        if (is_digit(ch)) {
            ns.dot_digits.push_back(static_cast<char>(ch));
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
        } else if (is_digit(ch)) {
            ns.exp_digits.push_back(static_cast<char>(ch));
            ns.state = NumberSubState::EXP_DIGIT;
        } else {
            this->unknown_char(ch, "expect digit or sign");
        }
    } else if (ns.state == NumberSubState::EXP_SIGNED) {
        if (is_digit(ch)) {
            ns.exp_digits.push_back(static_cast<char>(ch));
            ns.state = NumberSubState::EXP_DIGIT;
        } else {
            this->unknown_char(ch, "expect digit");
        }
    } else if (ns.state == NumberSubState::EXP_DIGIT) {
        if (is_digit(ch)) {
            ns.exp_digits.push_back(static_cast<char>(ch));
        } else {
            this->finish_num(ch);
        }
    } else {
        assert(!"Unreachable");
    }
}


void Scanner::st_string(CharConf::CharType ch) {
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
        } else if (ch == '\0') {
            this->unknown_char(ch, "received \\0");
        } else {
            // TODO: check the range of ch
            ss.value.push_back(ch);
        }
    } else if (ss.state == StringSubState::ESCAPE) {
        auto it = Scanner::escape_map.find(ch);
        if (it != Scanner::escape_map.end()) {
            ss.value.push_back(it->second);
            ss.state = StringSubState::NORMAL;
        } else if (ch == 'u') {
            ss.state = StringSubState::HEX;
        } else {
            assert(!"Unimplemented");
        }
    } else if (ss.state == StringSubState::HEX) {
        if (ss.hex.size() == 4) {
            int uch = strtol(ss.hex.data(), nullptr, 16);
            ss.value.push_back(static_cast<CharConf::CharType>(uch));
            ss.hex.clear();
            ss.state = StringSubState::NORMAL;
            this->refeed(ch);
        } else if (is_xdigit(ch)) {
            ss.hex.push_back(static_cast<char>(to_lower(ch)));
        } else {
            this->unknown_char(ch, "expect hex digit");
        }
    } else {
        assert(!"Unreachable");
    }
}


const map<CharConf::CharType, CharConf::CharType> Scanner::escape_map {
    {'b', '\b'},
    {'f', '\f'},
    {'n', '\n'},
    {'r', '\r'},
    {'t', '\t'},
    {'"', '"'},
    {'\\', '\\'},
    {'/', '/'},
};


void Scanner::exception(const string &msg, SourcePos start, SourcePos end) {
    if (!start.is_valid()) {
        start = this->start_pos;
    }
    if (!end.is_valid()) {
        end = this->cur_pos;
    }
    throw TokenizerError(msg, start, end);
}


void Scanner::unknown_char(CharConf::CharType ch, const string &additional) {
    string msg = "Unknown char: " + u8_encode({ch}); // TODO: escape ch
    if (!additional.empty()) {
        msg += ", " + additional;
    }
    this->exception(msg);
}


void Scanner::finish_num(CharConf::CharType ch) {
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
    for (auto ch : this->dot_digits) {
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
