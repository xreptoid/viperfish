#include "viperfish/json/loader.hpp"

#include <fstream>
#include <sstream>
#include <unordered_set>

#include <unistd.h>
#include <pwd.h>

namespace viperfish {

    json load_json_from_file(const std::string& file_path) {
        std::ifstream keys_file;
        keys_file.open(file_path);

        std::stringstream data_stream;
        data_stream << keys_file.rdbuf();
        std::string data = data_stream.str();

        std::unordered_set<char> symbols_to_strip = {' ', '\n', '\t'};

        int start_pos = 0;
        while (start_pos < data.size() && symbols_to_strip.count(data[start_pos])) {
            ++start_pos;
        }
        int end_pos = data.size() - 1;
        while (end_pos >= 0 && symbols_to_strip.count(data[end_pos])) {
            --end_pos;
        }

        data = data.substr(start_pos, end_pos - start_pos + 1);

        return json::parse(data);
    }

    json load_json_from_file_at_homedir(const std::string& file_path) {
        struct passwd *pw = getpwuid(getuid());
        std::string home_dir(pw->pw_dir);
        return load_json_from_file(home_dir + "/" + file_path);
    }
}