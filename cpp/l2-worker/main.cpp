#include <hiredis/hiredis.h>
#include <curl/curl.h>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <atomic>
#include <csignal>

#include "jsoncpp/json.h"
#include <prometheus/registry.h>
#include <prometheus/exposer.h>
#include <prometheus/counter.h>

std::atomic<bool> shutdown_flag(false);

// Prometheus registry
std::shared_ptr<prometheus::Registry> registry = std::make_shared<prometheus::Registry>();

// Prometheus counters
auto& l2_worker_requests_processed_total = prometheus::BuildCounter()
    .Name("l2_worker_requests_processed_total")
    .Help("Total number of requests processed by L2 worker")
    .Register(*registry);

auto& l2_worker_redis_operations_total = prometheus::BuildCounter()
    .Name("l2_worker_redis_operations_total")
    .Help("Total number of Redis operations performed by L2 worker")
    .Register(*registry);

auto& l2_worker_l2_calls_total = prometheus::BuildCounter()
    .Name("l2_worker_l2_calls_total")
    .Help("Total number of L2 server calls made by worker")
    .Register(*registry);

auto& l2_worker_redis_errors_total = prometheus::BuildCounter()
    .Name("l2_worker_redis_errors_total")
    .Help("Total number of Redis operation errors in L2 worker")
    .Register(*registry);

auto& l2_worker_l2_errors_total = prometheus::BuildCounter()
    .Name("l2_worker_l2_errors_total")
    .Help("Total number of L2 server call errors in worker")
    .Register(*registry);

auto& l2_worker_bytes_received_total = prometheus::BuildCounter()
    .Name("l2_worker_bytes_received_total")
    .Help("Total number of bytes received from Redis")
    .Register(*registry);

auto& l2_worker_bytes_sent_total = prometheus::BuildCounter()
    .Name("l2_worker_bytes_sent_total")
    .Help("Total number of bytes sent to Redis")
    .Register(*registry);

// Counter instances
prometheus::Counter& requests_processed_counter = l2_worker_requests_processed_total.Add({});
prometheus::Counter& redis_operations_counter = l2_worker_redis_operations_total.Add({});
prometheus::Counter& l2_calls_counter = l2_worker_l2_calls_total.Add({});
prometheus::Counter& redis_errors_counter = l2_worker_redis_errors_total.Add({});
prometheus::Counter& l2_errors_counter = l2_worker_l2_errors_total.Add({});
prometheus::Counter& bytes_received_counter = l2_worker_bytes_received_total.Add({});
prometheus::Counter& bytes_sent_counter = l2_worker_bytes_sent_total.Add({});

class L2Worker {
private:
    redisContext* redis;
    CURL* curl;
    std::string l2_server_url;

    static size_t write_callback(void* contents, size_t size, size_t nmemb, std::string* response) {
        size_t total_size = size * nmemb;
        response->append((char*)contents, total_size);
        return total_size;
    }

public:
    L2Worker(const std::string& redis_host, int redis_port, const std::string& server_url) 
        : l2_server_url(server_url) {
        
        redis = redisConnect(redis_host.c_str(), redis_port);
        if (redis == NULL || redis->err) {
            std::cerr << "Redis connection error: " << (redis ? redis->errstr : "can't allocate redis context") << std::endl;
            exit(1);
        }

        curl = curl_easy_init();
        if (!curl) {
            std::cerr << "CURL initialization failed" << std::endl;
            exit(1);
        }
    }

    ~L2Worker() {
        if (redis) {
            redisFree(redis);
        }
        if (curl) {
            curl_easy_cleanup(curl);
        }
    }

    std::string call_l2_server(const std::string& path, const std::string& body) {
        l2_calls_counter.Increment();

        std::string url = l2_server_url + path;
        std::string response_string;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

        if (!body.empty()) {
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body.length());
            struct curl_slist* headers = NULL;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        }

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            l2_errors_counter.Increment();
            return "{\"error\": \"Failed to call L2 server: " + std::string(curl_easy_strerror(res)) + "\"}";
        }

        return response_string;
    }

    void process_request(const std::string& request_json) {
        requests_processed_counter.Increment();
        bytes_received_counter.Increment(request_json.size());

        Json::Value request_data;
        Json::Reader reader;

        if (!reader.parse(request_json, request_data)) {
            std::cerr << "Failed to parse JSON request" << std::endl;
            return;
        }

        std::string method = request_data["method"].asString();
        if (method != "POST") {
            std::cout << "Skipping non-POST request: " << method << std::endl;
            return;
        }

        std::string request_id = request_data["id"].asString();
        std::string path = request_data["path"].asString();
        std::string body = request_data["body"].asString();

        std::cout << "Processing POST request: " << request_id << " path: " << path << "body:" << body << std::endl;

        // Call L2 server
        std::string l2_response = call_l2_server(path, body);

        // Prepare response for Redis
        Json::Value response_data;
        response_data["status_code"] = 200;
        response_data["headers"]["Content-Type"] = "application/json";

        Json::Value response_body;
        response_body["message"] = "Processed by C++ L2 Worker";
        response_body["language"] = "C++";
        response_body["request_id"] = request_id;
        response_body["l2_response"] = l2_response;
        response_body["timestamp"] = (Json::Int64)std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        response_data["body"] = response_body;

        // Store response in Redis
        Json::StreamWriterBuilder writer;
        std::string response_str = Json::writeString(writer, response_data);
        bytes_sent_counter.Increment(response_str.size());

        redis_operations_counter.Increment();
        redisReply* reply = (redisReply*)redisCommand(redis, "SETEX http:response:%s 60 %s",
                                                     request_id.c_str(), response_str.c_str());
        if (reply && reply->type != REDIS_REPLY_STATUS) {
            redis_errors_counter.Increment();
        }
        if (reply) freeReplyObject(reply);
    }

    void run() {
        std::cout << "C++ L2 Worker started. Waiting for requests..." << std::endl;

        while (!shutdown_flag) {
            redis_operations_counter.Increment();
            redisReply* reply = (redisReply*)redisCommand(redis, "BLPOP http:requests 10");

            if (reply && reply->type == REDIS_REPLY_ARRAY && reply->elements == 2) {
                std::string request_json = reply->element[1]->str;
                process_request(request_json);
                // Increment read counter
                redis_operations_counter.Increment();
                redisReply* incr_reply = (redisReply*)redisCommand(redis, "INCR stats:redis_reads");
                if (!(incr_reply && incr_reply->type == REDIS_REPLY_INTEGER)) {
                    redis_errors_counter.Increment();
                }
                if (incr_reply) freeReplyObject(incr_reply);
            } else if (reply && reply->type != REDIS_REPLY_ARRAY) {
                redis_errors_counter.Increment();
            }

            if (reply) freeReplyObject(reply);
            // std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        std::cout << "Shutting down gracefully..." << std::endl;
    }
};

int main() {
    // Set up signal handlers for graceful shutdown
    std::signal(SIGTERM, [](int){ shutdown_flag = true; });
    std::signal(SIGINT, [](int){ shutdown_flag = true; });

    std::string redis_host = "valkey";
    int redis_port = 6379;
    std::string l2_server_url = "http://l2-server:3000";

    // Start Prometheus exposer
    prometheus::Exposer exposer{"0.0.0.0:9091"};
    exposer.RegisterCollectable(registry);

    L2Worker worker(redis_host, redis_port, l2_server_url);

    std::cout << "C++ L2 Worker Prometheus metrics available at http://0.0.0.0:9091/metrics" << std::endl;
    worker.run();

    return 0;
}
