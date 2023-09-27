#ifndef VIPERFISH_STRINGS_HPP
#define VIPERFISH_STRINGS_HPP

#include <vector>
#include <string>

namespace viperfish {
    std::vector<std::string>
    split_string(const std::string &s, char delimiter);

    std::vector<std::string>
    split_string(const std::string& s, const std::string& delimiter);

    std::string
    join_strings(
            const std::vector<std::string> &strings,
            const std::string &delimiter
    );

    bool starts_with(const std::string& str, const std::string& prefix);
    bool ends_with(const std::string& str, const std::string& suffix);

    std::string to_upper(const std::string& s);
}

#endif
