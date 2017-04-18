#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "../exceptions.h"
#include "../unicode.h"
#include "../scanner.h"
#include "../parser.h"
#include "helper.h"


using namespace std;


enum class ValidateResult : int {
    SUCCESS = 0,
    UNICODE_ERROR = 10,
    TOKENIZE_ERROR,
    PARSE_ERROR,
};


string read_file(const string &path) {
    ifstream file(path);
    return string(istreambuf_iterator<char>(file), istreambuf_iterator<char>());
}


Node::Ptr parse(const vector<Token::Ptr> &tokens) {
    Parser parser;
    for (const Token::Ptr &tok : tokens) {
        parser.feed(*tok);
    }
    if (parser.is_finished()) {
        return parser.pop_result();
    } else {
        return Node::Ptr();
    }
}


ValidateResult validate_file(const string &path) {
    string content = read_file(path);

    ustring us;
    try {
        us = u8_decode(content.data());
    } catch (UnicodeError &exc) {
        cerr << "UnicodeError: " << exc.what() << endl;
        return ValidateResult::UNICODE_ERROR;
    }

    vector<Token::Ptr> tokens;
    try {
        tokens = get_tokens(us);
    } catch (TokenizerError &exc) {
        cerr << "TokenizerError: " << exc.what() << endl;
        return ValidateResult::TOKENIZE_ERROR;
    }

    Node::Ptr node;
    try {
        node = parse(tokens);
    } catch (ParserError &exc) {
        cerr << "ParserError: " << exc.what() << endl;
        return ValidateResult::PARSE_ERROR;
    }
    if (!node) {
        cerr << "Parser not finished" << endl;
        return ValidateResult::PARSE_ERROR;
    }

    string formated = format_node(*node);
    cout << formated << endl;
    return ValidateResult::SUCCESS;
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        cerr << "expect a file name as argument" << endl;
        return 5;
    }

    ValidateResult result = validate_file(argv[1]);
    return static_cast<int>(result);
}
