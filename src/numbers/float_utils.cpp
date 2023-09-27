#include "numbers/float_utils.hpp"

#include <string>
#include <sstream>
#include <iomanip>

namespace viperfish {

    std::size_t get_number_precision(const std::string& value_str) {
        auto pos = value_str.find('.');
        if (pos == std::string::npos) {
            return 0;
        }
        auto last_non_zero_pos = value_str.size() - 1;
        while (last_non_zero_pos > 0 && value_str[last_non_zero_pos] == '0') {
            --last_non_zero_pos;
        }
        return last_non_zero_pos - pos;
    }

    std::string number2string(double value, int precision) {
        std::stringstream ss;
        ss << std::setprecision(precision) << std::fixed << value;
        return ss.str();
    }
}