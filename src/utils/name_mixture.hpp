#ifndef VIPERFISH_UTILS_NAME_MIXTURE_HPP
#define VIPERFISH_UTILS_NAME_MIXTURE_HPP

#include <string>
#include <optional>

namespace viperfish::utils {
    
    class NameMixture {
    public:

        std::string _name;

        virtual std::string _get_app_name() const = 0;
        void set_name(const std::string value) { _name = value; }

    protected:
        std::string log_prefix(const std::string& method = "") const;
    };
}

#endif