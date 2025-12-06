#include <random>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <curl/curl.h>
#include <iostream>
#include "nlohmann/json.hpp"

class TraceLogger {

public:
    TraceLogger(const std::string& endpoint, const std::string& auth)
        : oo_trace_url(endpoint), basic_auth(auth) {
        curl_global_init(CURL_GLOBAL_ALL); // Инициализация libcurl (вызывать один раз в программе)
    }

    ~TraceLogger() {
        // curl_global_cleanup(); // Вызывать только при завершении всей программы!
    }

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
        CURL* curl = curl_easy_init();
        if (!curl) {
            std::cerr << "Failed to initialize curl for trace send\n";
            return;
        }

        // Настройка параметров
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 0L);
        curl_easy_setopt(curl, CURLOPT_FRESH_CONNECT, 0L);

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
        std::string payload_str = payload.dump();

        // Формируем заголовки (динамически, т.к. содержат basic_auth)
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        std::string auth_header = "Authorization: Basic " + basic_auth;
        headers = curl_slist_append(headers, auth_header.c_str());

        // Устанавливаем параметры для этого запроса
        curl_easy_setopt(curl, CURLOPT_URL, oo_trace_url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload_str.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(payload_str.size()));

        std::cout << " TraceLoger-body:" << payload_str << std::endl;

        // Выполняем запрос
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "Trace send failed: " << curl_easy_strerror(res)
                      << " (" << oo_trace_url << ")\n";
        }

        // Освобождаем заголовки
        curl_slist_free_all(headers);

        // Освобождаем curl
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

        for (auto& el : additional_attributes.items()) {
            attrs[el.key()] = el.value();
        }

        send_span(
            trace_id,
            span_id,
            "",
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
        thread_local std::random_device rd;
        thread_local std::mt19937 gen(rd());
        thread_local std::uniform_int_distribution<> dis(0, 15);

        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        for (size_t i = 0; i < len; ++i) {
            ss << std::setw(1) << dis(gen);
        }
        return ss.str();
    }
};
