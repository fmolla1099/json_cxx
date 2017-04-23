#ifndef ARGGEN_VALIDATOR_OPTION_H
#define ARGGEN_VALIDATOR_OPTION_H

// WARNING: Automatically generated code by arggen.py. Do not edit.

#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>


class ArgError : public std::runtime_error {
public:
    ArgError(const std::string &msg) : std::runtime_error(msg) {}
};


struct ValidatorOption {
    // options: ('file',), arg_type: ArgType.ONE
    std::string file = "-";
    // options: ('-i', '--interactive'), arg_type: ArgType.BOOL
    bool interactive = false;

    std::string to_string() const;
    bool operator==(const ValidatorOption &rhs) const;
    bool operator!=(const ValidatorOption &rhs) const;
    static ValidatorOption parse_args(const std::vector<std::string> &args);
    static ValidatorOption parse_argv(int argc, const char *argv[]);
};

#endif // ARGGEN_VALIDATOR_OPTION_H
