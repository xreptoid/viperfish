#ifndef INCLUDE_VIPERFISH_NUMBERS_FLOAT_UTILS_HPP
#define INCLUDE_VIPERFISH_NUMBERS_FLOAT_UTILS_HPP

#include <string>

namespace viperfish {

    std::size_t get_number_precision(const std::string& value_str);

    std::string number2string(double value, int precision);
}

#endif 
