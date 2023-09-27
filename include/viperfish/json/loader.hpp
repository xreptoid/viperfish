#ifndef INCLUDE_VIPERFISH_JSON_LOADER_HPP
#define INCLUDE_VIPERFISH_JSON_LOADER_HPP

#include "viperfish/json/json_obj.hpp"

namespace viperfish {
    json load_json_from_file(const std::string& file_path);
    json load_json_from_file_at_homedir(const std::string& file_path);
}

#endif 
