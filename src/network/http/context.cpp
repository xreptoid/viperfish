#ifndef VIPERFISH_CONNECTION_CONTEXT_CPP_
#define VIPERFISH_CONNECTION_CONTEXT_CPP_
#include "network/http/http_base.hpp"
#include <string>
#include <vector>
#include "json.hpp"
#include "strings.hpp"

namespace viperfish::network::http {

    Header Header::parse(const std::string& data) {
        int pos = data.find(':');
        if (pos == std::string::npos) {
            throw std::runtime_error("Invalid header " + data);
        }
        int value_start_pos = pos + 1;
        while (value_start_pos < data.size() && data[value_start_pos] == ' ') {
            ++value_start_pos;
        }
        if (value_start_pos >= data.size()) {
            throw std::runtime_error("Invalid header " + data);
        }
        return {
            .key = data.substr(0, pos),
            .value = data.substr(value_start_pos, data.size() - value_start_pos)
        };
    }

    std::string Headers::serialize() const {
        std::vector<std::string> serialized;
        for (const auto& h: headers) {
            serialized.push_back(h.serialize());
        }
        return join_strings(serialized, "\r\n");
    }

    Headers Headers::parse(const std::vector<std::string>& data) {
        Headers res;
        for (const auto& row: data) {
            res.add(Header::parse(row));
        }
        return res;
    }

    Headers Headers::from_json(const json& data) {
        Headers headers;
        for (const auto& h: data) {
            headers.add({
                .key = json_field_get<std::string>(h, "key"),
                .value = json_field_get<std::string>(h, "value"),
            });
        }
        return headers;
    }

    json Headers::to_json() const {
        std::vector<json> data;
        for (const auto& h: headers) {
            data.push_back(json({
                {"key", h.key},
                {"value", h.value},
            }));
        }
        return json(data);
    }

    std::optional<Header> Headers::find(const std::string& key) const {
        for (const auto& header: headers) {
            if (header.key == key) {
                return header;
            }
        }
        return {};
    }

    std::string QueryString::serialize() const {
        std::vector<std::string> serialized;
        for (const auto& p: params) {
            serialized.push_back(serialize_param(p));
        }

        return join_strings(serialized, "&");
    }

    Request::Request(
        const std::string &method,
        const std::string &host,
        int port,
        const std::string &path,
        const Headers& headers
    )
        : method(method)
        , host(host)
        , port(port)
        , path(path)
        , headers(headers + Headers({
            {"Host",   host},
            {"Accept", "*/*"}
        }))
    {}

    void Request::patch_prepared_with_headers(std::string &prepared) const {
        prepared.append(headers.serialize());
    }

    OptionsRequest::OptionsRequest(
        const std::string &host,
        int port,
        const std::string &path,
        const Headers &headers
    )
        : Request("OPTIONS", host, port, path, headers)
    {}

    std::string OptionsRequest::prepare() const {
        std::string prepared;
        prepared.append(method);
        prepared.append(" ");
        prepared.append(path);
        prepared.append(" HTTP/1.1\r\n");
        patch_prepared_with_headers(prepared);
        prepared.append("\r\n");
        prepared.append("\r\n");

        return prepared;
    }


    std::string OptionsRequest::getUrl() const {
        return get_proto() + host + ":" + std::to_string(port) + path;
    }

    GetRequest::GetRequest(
        const std::string &host,
        int port,
        const std::string &path,
        const Headers &headers,
        const QueryString &params
    )
        : Request("GET", host, port, path, headers)
        , params(params)
    {}

    std::string GetRequest::prepare() const {
        std::string prepared;
        prepared.append(method);
        prepared.append(" ");
        prepared.append(path);
        if (!params.empty()) {
            prepared.append("?");
            prepared.append(params.serialize());
        }
        prepared.append(" HTTP/1.1\r\n");
        patch_prepared_with_headers(prepared);
        prepared.append("\r\n");
        prepared.append("\r\n");

        return prepared;
    }

    std::string GetRequest::getUrl() const {
        std::string q;
        if (!params.empty()) {
            q.append("?");
            q.append(params.serialize());
        }
        return get_proto() + host + ":" + std::to_string(port) + path + q;
    }

    FormDataRequest::FormDataRequest(
            const std::string& method,
            const std::string &host,
            int port,
            const std::string &path,
            const Headers &headers,
            const QueryString &body
    )
        : Request(
            method, host, port, path,
            headers + Headers({Header{"Content-Type", "application/x-www-form-urlencoded"}}))
        , body(body)
    {}

    std::string FormDataRequest::prepare() const {
        std::string prepared;
        prepared.append(method + " " + path + " HTTP/1.1\r\n");
        std::string serialized_body = body.serialize();
        patch_prepared_with_headers(prepared);
        prepared.append("\r\n");
        prepared.append(Header({"Content-Length", std::to_string(serialized_body.size())}).serialize());
        prepared.append("\r\n");
        prepared.append("\r\n");
        prepared.append(serialized_body);
        prepared.append("\r\n");
        return prepared;
    }

    JsonRequest::JsonRequest(
            const std::string& method,
            const std::string& host,
            int port,
            const std::string& path,
            const Headers& headers,
            const json& body
    )
            : Request(
                method, host, port, path,
                headers + Headers({Header{"Content-Type", "application/json"}}))
            , body(body)
    {}

    std::string JsonRequest::prepare() const {
        std::string prepared;
        prepared.append(method + " " + path + " HTTP/1.1\r\n");
        std::string serialized_body = body.dump();
        patch_prepared_with_headers(prepared);
        prepared.append("\r\n");
        prepared.append(Header({"Content-Length", std::to_string(serialized_body.size())}).serialize());
        prepared.append("\r\n");
        prepared.append("\r\n");
        prepared.append(serialized_body);
        prepared.append("\r\n");
        return prepared;
    }
}
#endif
