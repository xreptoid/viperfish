#ifndef VIPERFISH_HTTP_BASE_HPP
#define VIPERFISH_HTTP_BASE_HPP
#include <string>
#include <vector>
#include <optional>
#include "viperfish/json.hpp"

namespace viperfish::network::http {

    class Route {
        public:
            std::string host;
            std::string hostname;
            int port;

            Route(const std::string& host, const std::string& hostname, int port)
                    : host(host) , hostname(hostname), port(port) {}
    };

    struct Header {
        std::string key;
        std::string value;

        std::string serialize() const { return key + ": " + value; }
        static Header parse(const std::string& data);
    };

    class Headers {
    public:
        std::vector<Header> headers;

        Headers(): headers({}) {}
        Headers(const std::vector<Header>& headers): headers(headers) {}
        void add(const Header& header) { headers.push_back(header); }
        std::string serialize() const;
        static Headers parse(const std::vector<std::string>& data);

        static Headers from_json(const json&);
        json to_json() const;

        std::optional<Header> find(const std::string&) const;

        Headers operator+(const Headers& other) const {
            std::vector<Header> new_headers = this->headers;
            for (const auto& h: other.headers) {
                new_headers.push_back(h);
            }
            return Headers(new_headers);
        }
    };

    struct QueryStringParam {
        const std::string key;
        const std::string value;
    };


    class QueryString {
    public:
        std::vector<QueryStringParam> params;
        QueryString(): params({}) {}
        QueryString(const std::vector<QueryStringParam>& params): params(params) {}
        void add(const QueryStringParam& param) { params.push_back(param); }
        std::string serialize() const;
        bool empty() const { return params.empty(); }

    private:
        std::string serialize_param(const QueryStringParam &param) const { return param.key + "=" + param.value; }
    };


    class Request {
    public:

        const std::string method;
        const std::string host;
        const int port;
        const std::string path;
        const Headers headers;

        bool ssl = true;

        Request(
                const std::string &method,
                const std::string &host,
                int port,
                const std::string &path,
                const Headers &headers
        );
        virtual ~Request() = default;

        virtual std::string prepare() const = 0;
        virtual std::string getUrl() const = 0;
        virtual std::string get_body() const { return ""; }
        virtual void set_ssl(bool value = true) { ssl = value; }
        virtual std::string get_proto() const { return ssl ? "https://" : "http://"; }

    protected:
        void patch_prepared_with_headers(std::string &prepared) const;
    };

    class OptionsRequest : public Request {
    public:

        OptionsRequest(
                const std::string &host,
                int port,
                const std::string &path,
                const Headers &headers
        );

        virtual std::string prepare() const override;
        virtual std::string getUrl() const override;
    };

    class GetRequest : public Request {
    public:

        const QueryString params;

        GetRequest(
                const std::string &host,
                int port,
                const std::string &path,
                const Headers &headers,
                const QueryString &params
        );

        virtual std::string prepare() const override;
        virtual std::string getUrl() const override;
    };


    class FormDataRequest : public Request {
    public:

        const QueryString body;

        FormDataRequest(
                const std::string& method,
                const std::string &host,
                int port,
                const std::string &path,
                const Headers &headers,
                const QueryString &body
        );

        std::string prepare() const override;
        std::string getUrl() const override { return get_proto() + host + ":" + std::to_string(port) + path; }
        std::string get_body() const override { return body.serialize(); }
    };

    class PostFormDataRequest : public FormDataRequest {
    public:
        PostFormDataRequest(
                const std::string &host,
                int port,
                const std::string &path,
                const Headers &headers,
                const QueryString &body
        )
                : FormDataRequest("POST", host, port, path, headers, body)
        {}
    };


    class DeleteFormDataRequest : public FormDataRequest {
    public:
        DeleteFormDataRequest(
                const std::string &host,
                int port,
                const std::string &path,
                const Headers &headers,
                const QueryString &body
        ) : FormDataRequest("DELETE", host, port, path, headers, body)
        {}
    };


    class JsonRequest : public Request {
    public:
        json body;

        JsonRequest(
                const std::string& method,
                const std::string& host,
                int port,
                const std::string& path,
                const Headers& headers,
                const json& body
        );

        std::string prepare() const override;
        std::string getUrl() const override { return get_proto() + host + ":" + std::to_string(port) + path; }
        std::string get_body() const override { return body.dump(); }
    };

    class PostJsonRequest: public JsonRequest {
    public:
        PostJsonRequest(
                const std::string& host,
                int port,
                const std::string& path,
                const Headers& headers,
                const json& body
        )
            : JsonRequest("POST", host, port, path, headers, body)
        {}
    };

    class PutJsonRequest: public JsonRequest {
    public:
        PutJsonRequest(
                const std::string& host,
                int port,
                const std::string& path,
                const Headers& headers,
                const json& body
        )
                : JsonRequest("PUT", host, port, path, headers, body)
        {}
    };

    class DeleteJsonRequest: public JsonRequest {
    public:
        DeleteJsonRequest(
                const std::string& host,
                int port,
                const std::string& path,
                const Headers& headers,
                const json& body
        )
                : JsonRequest("DELETE", host, port, path, headers, body)
        {}
    };

    class Response {
    public:
        std::string buf;
        Headers headers;
        std::string host;
        int ssl_read_error_code;

        Response(
                const std::string& buf,
                const Headers& headers,
                const std::string& host,
                int ssl_read_error_code = 0
        )
            : buf(buf)
            , headers(headers)
            , host(host)
            , ssl_read_error_code(ssl_read_error_code)
        {}

        static Response ok(const std::string& buf) { return Response(buf, Headers(), ""); }
        static Response ok(const std::string& buf, const Headers& headers) { return Response(buf, headers, ""); }
        static Response ok(const std::string& buf, const Headers& headers, const std::string& host) {
            return Response(buf, headers, host);
        }
        static Response sslError(int error_code) { return Response("", Headers(), "", error_code); }
        static Response sslError(const std::string& buf, int error_code) { return Response(buf, Headers(), "", error_code); }

        json getJson() const { return json::parse(buf); }
    };
}
#endif 
