#include <algorithm>
#include <cassert>
#include <cctype>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "../exceptions.h"
#include "../unicode.h"
#include "../scanner.h"
#include "../parser.h"
#include "../sourcepos.h"
#include "helper.h"
#include "validator_option.h"


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


// http://stackoverflow.com/a/202097/3886899
string read_stdin() {
    // don't skip the whitespace while reading
    cin >> std::noskipws;

    // use stream iterators to copy the stream to a string
    istream_iterator<char> it(cin);
    istream_iterator<char> end;
    return string(it, end);
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
    string content;
    if (path == "-") {
        content = read_stdin();
    } else {
        content = read_file(path);
    }

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


class Validator {
public:
    void feed_line(const string &line) {
        try {
            this->feed_line_unchecked(line);
        } catch (exception &) {
            this->reset();
            throw;
        }
    }

    bool is_finished() const {
        return this->parser.is_finished();
    }

    Node::Ptr pop_result() {
        Node::Ptr node = parser.pop_result();
        this->reset();
        return node;
    }

    void reset() {
        this->scanner = Scanner();
        this->parser = Parser();
    }

private:
    void feed_line_unchecked(const string &line) {
        ustring us = u8_decode(line.data());
        us.push_back('\n');
        for (auto ch : us) {
            this->scanner.feed(ch);
            Token::Ptr tok;
            while ((tok = this->scanner.pop())) {
                this->parser.feed(*tok);
            }
        }
    }

    Scanner scanner;
    Parser parser;
};


bool is_empty_line(const string &line) {
    return all_of(line.cbegin(), line.cend(), [](const char ch) { return isspace(ch); });
}


void highlight_last_line(
    const SourcePos &start, const SourcePos &end, bool leading_caret = true, size_t prompt_len = 0)
{
    if (prompt_len > 0) {
        cerr << string(prompt_len - 1, '!') << " ";
    }
    assert(start.is_valid());
    assert(end.is_valid());
    assert(end.rowno >= start.rowno);
    cerr << string((size_t)start.rowno, ' ');
    if (leading_caret) {
        cerr << "^";
    }
    cerr << string((size_t)(end.rowno - start.rowno), '~');
    if (!leading_caret) {
        cerr << "^";
    }
    cerr << endl;
}


void interactive_repl() {
    string line;
    string prompt;
    bool is_ready = true;
    Validator val;

    while (cin) {
        // print prompt
        if (is_ready) {
            prompt = "<<< ";
        } else {
            prompt = "... ";
        }
        cout << prompt;

        getline(cin, line);
        // skip empty line
        if (is_ready && is_empty_line(line)) {
            continue;
        }

        try {
            val.feed_line(line);
        } catch (TokenizerError &exc) {
            highlight_last_line(exc.start, exc.end, false, prompt.size());
            cerr << "TokenizerError: " << exc.what() << endl;
            goto ERROR;
        } catch(ParserError &exc) {
            highlight_last_line(exc.start, exc.end, true, prompt.size());
            cerr << "ParserError: " << exc.what() << endl;
            goto ERROR;
        } catch (exception &exc) {
            cerr << typeid(exc).name() << ": " << exc.what() << endl;
            goto ERROR;
        }

        if (val.is_finished()) {
            is_ready = true;

            stringstream buf;
            Formatter().format(buf, *val.pop_result());

            string outline;
            while (!buf.eof()) {
                getline(buf, outline);
                cout << ">>> " << outline << endl;
            }
        } else {
            is_ready = false;
        }
        continue;

    ERROR:
        val.reset();
        is_ready = true;
    }

    // print a new line after ctrl-d
    cout << endl;
}


int main(int argc, const char *argv[]) {
    ValidatorOption option;
    try {
        option = ValidatorOption::parse_argv(argc, argv);
    } catch (ArgError &exc) {
        cerr << "ArgError: " << exc.what() << endl;
        return 5;
    }

    if (option.interactive) {
        if (option.file != "-") {
            cerr << "can not specify argument in interactive mode" << endl;
            return 6;
        }
        interactive_repl();
        return 0;
    } else {
        ValidateResult result = validate_file(option.file);
        return static_cast<int>(result);
    }
}
