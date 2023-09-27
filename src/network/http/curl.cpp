#ifndef VIPERFISH_CURL_CPP_
#define VIPERFISH_CURL_CPP_
#include "network/http/http.hpp"
#include <iostream>
#include <string>
#include <curl/curl.h>

namespace viperfish::network::http {

    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
        ((std::string *) userp)->append((char *) contents, size * nmemb);
        return size * nmemb;
    }

    Response request_get(
        const std::string& url,
        const QueryString& params,
        const Headers& headers
    ) {
        CURL *curl;
        CURLcode res;
        std::string readBuffer;

        curl = curl_easy_init();
        if (curl) {
            std::string url_with_params = url + (params.empty() ? "" : ("?" + params.serialize()));

            struct curl_slist *curl_headers = NULL;
            for (auto header: headers.headers) {
                curl_headers = curl_slist_append(curl_headers, header.serialize().c_str());
            }

            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers);
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);

            return Response::ok(readBuffer);
        }

        return Response::sslError(-1); // FIXME
    }

    Response request_get(const GetRequest& request) {
        return request_get(request.getUrl(), request.params, request.headers);
    }

    Response request_form_data(const FormDataRequest &request) {
        CURL *curl;
        CURLcode res;
        std::string readBuffer;

        curl = curl_easy_init();

        if (curl) {
            struct curl_slist *curl_headers = NULL;

            // for c_str()
            std::string url = request.getUrl();
            std::string serialized_body = request.body.serialize();

            for (auto header: request.headers.headers) {
                curl_headers = curl_slist_append(curl_headers, header.serialize().c_str());
            }

            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers);
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            if (request.method == "POST") {
                curl_easy_setopt(curl, CURLOPT_POST, 1);
            }
            else {
                curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, request.method.c_str());
            }
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, serialized_body.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

            res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);

            return Response::ok(readBuffer);
        }

        return Response::sslError(-1); // FIXME
    }


    Response request_json(const JsonRequest& request) {
        CURL *curl;
        CURLcode res;
        std::string readBuffer;

        curl = curl_easy_init();

        if (curl) {
            struct curl_slist *curl_headers = NULL;

            // for c_str()
            std::string url = request.getUrl();
            std::string serialized_body = request.body.dump();

            for (auto header: request.headers.headers) {
                curl_headers = curl_slist_append(curl_headers, header.serialize().c_str());
            }

            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers);
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            if (request.method == "POST") {
                curl_easy_setopt(curl, CURLOPT_POST, 1);
            }
            else {
                curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, request.method.c_str());
            }
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, serialized_body.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

            res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);

            return Response::ok(readBuffer);
        }

        return Response::sslError(-1); // FIXME
    }

}

#endif
