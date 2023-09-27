#ifndef INCLUDE_VIPERFISH_JSON_GETTER_HPP
#define INCLUDE_VIPERFISH_JSON_GETTER_HPP
#include <iostream>
#include <iostream>
#ifndef _GNU_SOURCE 
#   define _GNU_SOURCE 
#endif 
#include <boost/stacktrace.hpp>
#include "json/json_obj.hpp"

namespace viperfish {

    template<typename T>
    std::optional<T> json_field_get_o(const viperfish::json& obj, const std::string& key) {
        auto it = obj.find(key);
        if (it == obj.end()) {
            return {};
        }
        return {*it};
    }

    template<typename T>
    T json_field_get(const viperfish::json& obj, const std::string& key) {
        auto o = json_field_get_o<T>(obj, key);
        if (!o.has_value()) {
            std::cout << boost::stacktrace::stacktrace() << std::endl;;
            throw std::runtime_error("No key " + key + " in json");
        }
        return *o;
    }

    template<typename T>
    T json_field_get(const viperfish::json& obj, const std::string& key, const T& default_value) {
        auto o = json_field_get_o<T>(obj, key);
        if (!o.has_value()) {
            return default_value;
        }
        return *o;
    }
}
#endif 
