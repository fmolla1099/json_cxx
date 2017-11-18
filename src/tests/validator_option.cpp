#include <cstdlib>   // atol
#include <cstring>   // strlen
#include <string>    // to_string
#include "validator_option.h"

// WARNING: Automatically generated code by arggen.py. Do not edit.


bool ValidatorOption::operator==(const ValidatorOption &rhs) const {
    return std::tie(this->files, this->interactive) \
        == std::tie(rhs.files, rhs.interactive);
}
bool ValidatorOption::operator!=(const ValidatorOption &rhs) const {
    return !(*this == rhs);
}


std::string ValidatorOption::to_string() const {
    std::string ans = "<ValidatorOption";
    ans += " interactive=";
    ans += this->interactive ? "true" : "false";
    ans += " files=";
    for (const auto &item : this->files) {
        ans += item + ",";
    }
    return ans + ">";
}


ValidatorOption ValidatorOption::parse_args(const std::vector<std::string> &args) {
    ValidatorOption ans {};   // initialized
    int position_count = 0;
    // required options
    for (size_t i = 0; i < args.size(); i++) {
        const std::string &piece = args[i];
        if (piece.size() > 2 && piece[0] == '-' && piece[1] == '-') {
            // long options
            if (piece == "--interactive") {
                ans.interactive = true;
            } else {
                throw ArgError("Unknown option: " + piece);
            }
        } else if (piece.size() >= 2 && piece[0] == '-') {
            // short options
            for (auto it = piece.begin() + 1; it != piece.end(); ++it) {
                if (*it == 'i') {
                    ans.interactive = true;
                } else {
                    throw ArgError("Unknown flag: " + std::string(1, *it));
                }
            }
        } else {
            // positional args
            ans.files.emplace_back(piece);
            position_count++;
        }
    }

    // check required options
    // check positional args
    if (position_count < 0) {
        throw ArgError("expect more argument");
    }
    return ans;
}


ValidatorOption ValidatorOption::parse_argv(int argc, const char *const argv[]) {
    std::vector<std::string> args;
    for (int i = 1; i < argc; i++) {
        args.emplace_back(argv[i]);
    }
    return ValidatorOption::parse_args(args);
}
