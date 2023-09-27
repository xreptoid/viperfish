#include "viperfish/utils/name_mixture.hpp"
#include <string>
#include <sstream>
#include <iomanip>
#include "viperfish/timestamp.hpp"

namespace viperfish::utils {

    std::string NameMixture::log_prefix(const std::string& method) const {
        std::stringstream ss;
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << '.';
        auto ms = get_current_ts() % 1000;
        if (ms < 10) {
            ss << '0';
        }
        if (ms < 100) {
            ss << '0';
        }
        ss << ms << ' ' << _get_app_name();
        if (!_name.empty()) {
            ss << '(' << _name << ')';
        }
        if (!method.empty()) {
            ss << '.' << method;
        }
        ss << ": ";
        return ss.str();
    }
}