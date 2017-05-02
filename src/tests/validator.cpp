#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#include "../exceptions.h"
#include "../unicode.h"
#include "../scanner.h"
#include "../parser.h"
#include "../formatter.h"
#include "../sourcepos.h"
#include "validator_option.h"


using namespace std;


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
        this->scanner.reset();
        this->parser.reset();
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
    assert(start.is_valid());
    assert(end.is_valid());
    assert(end.rowno >= start.rowno);

    if (prompt_len > 0) {
        cerr << string(prompt_len - 1, '!') << " ";
    }

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
        } catch (exception &exc) {
            try {
                throw;
            } catch (TokenizerError &exc) {
                highlight_last_line(exc.start, exc.end, false, prompt.size());
                cerr << "TokenizerError: " << exc.what() << endl;
            } catch(ParserError &exc) {
                highlight_last_line(exc.start, exc.end, true, prompt.size());
                cerr << "ParserError: " << exc.what() << endl;
            } catch (exception &exc) {
                cerr << typeid(exc).name() << ": " << exc.what() << endl;
            }

            // prepare for next json input
            val.reset();
            is_ready = true;
            continue;
        }

        if (val.is_finished()) {
            is_ready = true;

            // pretty print json
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
    }

    // print a new line after ctrl-d
    cout << endl;
}


bool path_exists(const string &path) {
    return access(path.data(), F_OK) == 0;
}


bool path_is_dir(const string &path) {
    struct stat buf;
    if (stat(path.data(), &buf) != 0) {
        return false;
    } else {
        return S_ISDIR(buf.st_mode);
    }
}


string path_join(const string &dir_name, const string &file_name) {
    if (!dir_name.empty() && dir_name.back() != '/') {
        return dir_name + "/" + file_name;
    } else {
        return dir_name + file_name;
    }
}


vector<string> path_list_dir(const string &path) {
    vector<string> ans;

    struct dirent *entry;
    DIR *dir = opendir(path.data());
    if (dir == nullptr) {
        perror("Can not open dir");
        return ans;
    }

    while ((entry = readdir(dir)) != nullptr) {
        string file_name = entry->d_name;
        if (file_name != "." && file_name != "..") {
            ans.emplace_back(path_join(path, file_name));
        }
    }
    closedir(dir);
    return ans;
}


enum class ValidateResult : int {
    SUCCESS = 0,
    UNICODE_ERROR = 10,
    TOKENIZE_ERROR,
    PARSE_ERROR,
    FILE_ERROR,
    UNKNOWN,
};


ValidateResult validate_stream(istream &input) {
    Validator val;
    string line;

    while (getline(input, line)) {
        try {
            val.feed_line(line);
        } catch (UnicodeError &exc) {
            cerr << "UnicodeError: " << exc.what() << endl;
            return ValidateResult::UNICODE_ERROR;
        } catch (TokenizerError &exc) {
            cerr << "TokenizerError: " << exc.what() << endl;
            return ValidateResult::TOKENIZE_ERROR;
        } catch (ParserError &exc) {
            cerr << "ParserError: " << exc.what() << endl;
            return ValidateResult::PARSE_ERROR;
        } catch (exception &exc) {
            cerr << "Unknown exception: " << typeid(exc).name() << ": " << exc.what() << endl;
            return ValidateResult::UNKNOWN;
        } catch (...) {
            cerr << "Unknown non-exeption caught" << endl;
            return ValidateResult::UNKNOWN;
        }
    }

    if (!val.is_finished()) {
        cerr << "Parser not finished" << endl;
        return ValidateResult::PARSE_ERROR;
    }

    // success
    cout << val.pop_result()->repr() << endl;
    return ValidateResult::SUCCESS;
}


#define CHECK_EXISTS(path) \
if (!path_exists(path)) { \
    perror("Path not exists"); \
    return ValidateResult::FILE_ERROR; \
}


ValidateResult validate_file(const string &path) {
    if (path == "-") {
        return validate_stream(cin);
    } else {
        CHECK_EXISTS(path);
        ifstream input(path, std::ios::in);
        if (!input) {
            cerr << "Can not open file: " << path << endl;
            return ValidateResult::FILE_ERROR;
        } else {
            return validate_stream(input);
        }
    }
}


ValidateResult validate_directory(const string &path) {
    CHECK_EXISTS(path);
    ValidateResult result = ValidateResult::SUCCESS;
    for (const string &sub_path : path_list_dir(path)) {
        ValidateResult sub_result;
        if (path_is_dir(sub_path)) {
            cerr << "enter dir: " << sub_path << endl;
            sub_result = validate_directory(sub_path);
            cerr << "leave dir: " << sub_path << endl;
        } else {
            cerr << "file: " << sub_path << endl;
            sub_result = validate_file(sub_path);
        }
        if (sub_result != ValidateResult::SUCCESS) {
            result = sub_result;
        }
    }
    return result;
}


ValidateResult validate_path(const string &path) {
    CHECK_EXISTS(path);
    if (path_is_dir(path)) {
        return validate_directory(path);
    } else {
        return validate_file(path);
    }
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
        if (!(option.files.empty() || option.files == vector<string> {"-"})) {
            cerr << "can not specify argument in interactive mode" << endl;
            return 6;
        }
        interactive_repl();
        return 0;
    } else {
        ValidateResult result = ValidateResult::SUCCESS;
        for (const string &file : option.files) {
            if (option.files.size() > 1) {
                cerr << "file: " << file << endl;
            }

            ValidateResult sub_result = validate_path(file);
            if (sub_result != ValidateResult::SUCCESS) {
                result = sub_result;
            }
        }
        return static_cast<int>(result);
    }
}
