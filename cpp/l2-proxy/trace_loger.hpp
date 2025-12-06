#include <random>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <curl/curl.h>
#include "nlohmann/json.hpp"

class TraceLogger {
public:
    TraceLogger(const std::string& endpoint, const std::string& auth)
        : oo_trace_url(endpoint), basic_auth(auth) {}

    std::string generate_trace_id() {
        return random_hex(32);
    }

    std::string generate_span_id() {
        return random_hex(16);
    }

    void send_span(
        const std::string& trace_id,
        const std::string& span_id,
        const std::string& parent_span_id,
        const std::string& name,
        uint64_t start_us,
        uint64_t end_us,
        const std::string& service_name,
        const nlohmann::json& attributes = {}
    ) {
        nlohmann::json span = {
            {"trace_id", trace_id},
            {"span_id", span_id},
            {"parent_span_id", parent_span_id},
            {"name", name},
            {"start_time", start_us},
            {"end_time", end_us},
            {"service_name", service_name},
            {"attributes", attributes}
        };

        nlohmann::json payload = nlohmann::json::array({span});

        // Отправка через libcurl (аналогично логам)
        CURL* curl = curl_easy_init();
        if (!curl) return;

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, ("Authorization: Basic " + basic_auth).c_str());

        std::string payload_str = payload.dump();

        curl_easy_setopt(curl, CURLOPT_URL, oo_trace_url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload_str.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, payload_str.size());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "Trace send failed: " << curl_easy_strerror(res) << " (" << oo_trace_url << ")"<< "\n";
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    void log_request(
        const std::string& method,
        const std::string& url,
        int status_code,
        uint64_t start_us,
        uint64_t end_us,
        const std::string& service_name,
        const std::string& request_id = "",
        const nlohmann::json& additional_attributes = {}
    ) {
        std::string trace_id = generate_trace_id();
        std::string span_id = generate_span_id();

        nlohmann::json attrs = {
            {"http.method", method},
            {"http.url", url},
            {"http.status_code", status_code}
        };

        if (!request_id.empty()) {
            attrs["request.id"] = request_id;
        }

        // Merge additional attributes
        for (auto& el : additional_attributes.items()) {
            attrs[el.key()] = el.value();
        }

        send_span(
            trace_id,
            span_id,
            "", // root span
            "HTTP " + method + " " + url,
            start_us,
            end_us,
            service_name,
            attrs
        );
    }

private:
    std::string oo_trace_url;
    std::string basic_auth;

    std::string random_hex(size_t len) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 15);
        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        for (size_t i = 0; i < len; ++i) {
            ss << std::setw(1) << dis(gen);
        }
        return ss.str();
    }
};


