#ifndef VIPERFISH_HTTP_HPP
#define VIPERFISH_HTTP_HPP
#include <string>
#include <vector>
#include "http_base.hpp"
#include "json.hpp"

namespace viperfish::network::http {

    Response request_get(const GetRequest&);
    Response request_get(const std::string& url, const QueryString& = {}, const Headers& = {});
    Response request_form_data(const FormDataRequest&);
    Response request_json(const JsonRequest&);
}

#endif 
