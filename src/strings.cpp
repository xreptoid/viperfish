#ifndef VIPERFISH_STRINGS_CPP_
#define VIPERFISH_STRINGS_CPP_

#include "strings.hpp"


#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <iterator>


namespace viperfish {

    std::vector<std::string>
    split_string(const std::string& s, char delimiter)
    {
        std::stringstream ss(s);
        std::string item;
        std::vector<std::string> items;
        while (std::getline(ss, item, delimiter)) {
           items.push_back(item);
        }
        return items;
    }

    std::vector<std::string>
    split_string(const std::string& s, const std::string& delimiter)
    {
        std::vector<std::string> res;
        std::size_t prev_pos = 0;
        while (true) {
            auto pos = s.find(delimiter, prev_pos);
            if (pos == std::string::npos) {
                res.push_back(s.substr(prev_pos, s.size() - prev_pos));
                break;
            }
            res.push_back(s.substr(prev_pos, pos - prev_pos));
            prev_pos = pos + delimiter.size();
        }
        return res;
    }

    std::string
    join_strings(
        const std::vector<std::string>& strings,
        const std::string& delimiter
    )
    {
        std::stringstream ss;
        copy(strings.begin(), strings.end(), std::ostream_iterator<std::string>(ss, delimiter.c_str()));
        auto s = ss.str();
        if (!s.empty() && !delimiter.empty()) {
            s.resize(s.size() - 1);
        }
        return s;
    }

    bool
    starts_with(const std::string& str, const std::string& prefix) {
        return str.substr(0, prefix.size()) == prefix;
    }

    bool
    ends_with(const std::string& str, const std::string& suffix) {
        return str.substr(str.size() - suffix.size(), suffix.size()) == suffix;
    }

    std::string to_upper(const std::string& s) {
        std::string res;
        for (const char& c: s) {
            char cc = c;
            if ('a' <= cc && cc <= 'z') {
                cc += 'A' - 'a';
            }
            res += cc;
        }
        return res;
    }
}

#endif
