#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <random>
#include <string>
#include <ctime>
#include <unistd.h>
#include <mutex>
#include <cstdlib>
#include <csignal>

#include "CivetServer.h"
#include <hiredis/hiredis.h>

#include <cpprest/json.h>
using namespace web;

#include <prometheus/registry.h>
#include <prometheus/exposer.h>
#include <prometheus/counter.h>

static int g_exit_flag = 0;

// Prometheus registry
std::shared_ptr<prometheus::Registry> registry = std::make_shared<prometheus::Registry>();

// Prometheus counters
auto& l2_proxy_client_requests_total = prometheus::BuildCounter()
    .Name("l2_proxy_client_requests_total")
    .Help("Total number of client requests received")
    .Register(*registry);

auto& l2_proxy_redis_requests_total = prometheus::BuildCounter()
    .Name("l2_proxy_redis_requests_total")
    .Help("Total number of Redis operations performed")
    .Register(*registry);

auto& l2_proxy_client_request_errors_total = prometheus::BuildCounter()
    .Name("l2_proxy_client_request_errors_total")
    .Help("Total number of client request errors")
    .Register(*registry);

auto& l2_proxy_redis_errors_total = prometheus::BuildCounter()
    .Name("l2_proxy_redis_errors_total")
    .Help("Total number of Redis operation errors")
    .Register(*registry);

auto& l2_proxy_bytes_received_total = prometheus::BuildCounter()
    .Name("l2_proxy_bytes_received_total")
    .Help("Total number of bytes received from clients")
    .Register(*registry);

auto& l2_proxy_bytes_sent_total = prometheus::BuildCounter()
    .Name("l2_proxy_bytes_sent_total")
    .Help("Total number of bytes sent to clients")
    .Register(*registry);

// Counter instances
prometheus::Counter& client_requests_counter = l2_proxy_client_requests_total.Add({});
prometheus::Counter& redis_requests_counter = l2_proxy_redis_requests_total.Add({});
prometheus::Counter& client_errors_counter = l2_proxy_client_request_errors_total.Add({});
prometheus::Counter& redis_errors_counter = l2_proxy_redis_errors_total.Add({});
prometheus::Counter& bytes_received_counter = l2_proxy_bytes_received_total.Add({});
prometheus::Counter& bytes_sent_counter = l2_proxy_bytes_sent_total.Add({});

void signal_handler(int signum) {
    std::cout << "Received signal " << signum << ", exiting..." << std::endl;
    g_exit_flag = 1;
}

class HealthHandler : public CivetHandler {
private:
    redisContext* redis;
    std::mutex redis_mutex;

public:
    HealthHandler(redisContext* r) : redis(r) {}

    bool handleGet(CivetServer *server, struct mg_connection *conn) {
        std::lock_guard<std::mutex> lock(redis_mutex);
        if (redis && redis->err) {
            mg_printf(conn, "HTTP/1.1 503 Service Unavailable\r\nContent-Type: text/plain\r\n\r\nRedis unavailable");
            return true;
        }

        redisReply* reply = (redisReply*)redisCommand(redis, "PING");
        redis_requests_counter.Increment();
        if (reply && reply->type == REDIS_REPLY_STATUS) {
            mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nOK");
        } else {
            redis_errors_counter.Increment();
            mg_printf(conn, "HTTP/1.1 503 Service Unavailable\r\nContent-Type: text/plain\r\n\r\nRedis unavailable");
        }
        if (reply) freeReplyObject(reply);
        return true;
    }
};

class StatsHandler : public CivetHandler {
private:
    redisContext* redis;
    std::mutex redis_mutex;

public:
    StatsHandler(redisContext* r) : redis(r) {}

    bool handleGet(CivetServer *server, struct mg_connection *conn) {
        std::lock_guard<std::mutex> lock(redis_mutex);
        if (!redis || redis->err) {
            mg_printf(conn, "HTTP/1.1 503 Service Unavailable\r\nContent-Type: text/plain\r\n\r\nRedis unavailable");
            return true;
        }

        redisReply* writes_reply = (redisReply*)redisCommand(redis, "GET stats:redis_writes");
        redis_requests_counter.Increment();
        if (!(writes_reply && writes_reply->type == REDIS_REPLY_STRING)) {
            redis_errors_counter.Increment();
        }

        redisReply* reads_reply = (redisReply*)redisCommand(redis, "GET stats:redis_reads");
        redis_requests_counter.Increment();
        if (!(reads_reply && reads_reply->type == REDIS_REPLY_STRING)) {
            redis_errors_counter.Increment();
        }

        long long writes = 0;
        long long reads = 0;

        if (writes_reply && writes_reply->type == REDIS_REPLY_STRING) {
            writes = atoll(writes_reply->str);
        }
        if (reads_reply && reads_reply->type == REDIS_REPLY_STRING) {
            reads = atoll(reads_reply->str);
        }

        json::value stats;
        stats[U("redis_writes")] = json::value::number(writes);
        stats[U("redis_reads")] = json::value::number(reads);

        std::string stats_json = utility::conversions::to_utf8string(stats.serialize());
        mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n%s", stats_json.c_str());

        if (writes_reply) freeReplyObject(writes_reply);
        if (reads_reply) freeReplyObject(reads_reply);
        return true;
    }
};

class RequestHandler : public CivetHandler {
private:
    redisContext* redis;
    std::mutex redis_mutex;
    bool use_sequential_id = true;
    long long request_id_counter = 0;

    std::string generate_uuid() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 15);
        std::stringstream ss;
        ss << std::hex;
        for (int i = 0; i < 32; i++) {
            ss << dis(gen);
        }
        return ss.str();
    }

    std::string generate_sequential_id() {
        std::lock_guard<std::mutex> lock(redis_mutex);
        request_id_counter++;
        return std::to_string(request_id_counter);
    }

    void save_counter() {
        std::lock_guard<std::mutex> lock(redis_mutex);
        redisReply* reply = (redisReply*)redisCommand(redis, "SET request_id_counter %lld", request_id_counter);
        redis_requests_counter.Increment();
        if (!(reply && reply->type == REDIS_REPLY_STATUS)) {
            redis_errors_counter.Increment();
        }
        if (reply) freeReplyObject(reply);
    }

public:
    RequestHandler(redisContext* r) : redis(r), request_id_counter(0) {
        // const char* env = std::getenv("USE_SEQUENTIAL_REQUEST_ID");
        // use_sequential_id = env && std::string(env) == "true";

        // Load counter from Redis
        std::lock_guard<std::mutex> lock(redis_mutex);
        redisReply* reply = (redisReply*)redisCommand(redis, "GET request_id_counter");
        redis_requests_counter.Increment();
        if (reply && reply->type == REDIS_REPLY_STRING) {
            request_id_counter = atoll(reply->str);
        } else if (reply && reply->type == REDIS_REPLY_NIL) {
            request_id_counter = 0;
        } else {
            redis_errors_counter.Increment();
            std::cerr << "Failed to load request_id_counter from Redis, starting from 0" << std::endl;
            request_id_counter = 0;
        }
        if (reply) freeReplyObject(reply);
    }

    ~RequestHandler() {
        save_counter();
    }

    bool handleGet(CivetServer *server, struct mg_connection *conn) {
        return handle_request(server, conn, "GET", "");
    }

    bool handlePost(CivetServer *server, struct mg_connection *conn) {
        const struct mg_request_info *req_info = mg_get_request_info(conn);
        std::string body;
        if (req_info->content_length > 0) {
            char* buffer = new char[req_info->content_length + 1];
            int read_len = mg_read(conn, buffer, req_info->content_length);
            if (read_len > 0) {
                buffer[read_len] = '\0';
                body = std::string(buffer);
            }
            delete[] buffer;
        }
        bytes_received_counter.Increment(body.size());
        return handle_request(server, conn, "POST", body);
    }

    bool handle_request(CivetServer *server, struct mg_connection *conn, const std::string& method, const std::string& body = "") {
        client_requests_counter.Increment();

        const struct mg_request_info *req_info = mg_get_request_info(conn);
        std::string path = req_info->request_uri ? req_info->request_uri : "/";

        std::string request_id = use_sequential_id ? generate_sequential_id() : generate_uuid();

        // Prepare request data for Redis
        // std::string request_data = "{\"id\": \"" + request_id + "\", \"method\": \"" + method + "\", \"path\": \"" + path + "\"}";

            // Prepare request data for Redis
        json::value request_data;
        request_data[U("id")] = json::value::string(utility::conversions::to_string_t(request_id));
        request_data[U("method")] = json::value::string(utility::conversions::to_string_t(method));
        request_data[U("path")] = json::value::string(utility::conversions::to_string_t(path));
        if (!body.empty()) {
            request_data[U("body")] = json::value::string(utility::conversions::to_string_t(body));
        }
        std::cout << "request_data: " << request_data << std::endl;

        bool redis_push_success = false;
        // Push to Redis queue
        if (redis && !redis->err) {
            std::lock_guard<std::mutex> lock(redis_mutex);
            std::string request_json = utility::conversions::to_utf8string(request_data.serialize());
            redisReply* reply = (redisReply*)redisCommand(redis, "RPUSH http:requests %s", request_json.c_str());
            redis_requests_counter.Increment();
            if (reply && reply->type == REDIS_REPLY_INTEGER) {
                redis_push_success = true;
            } else {
                redis_errors_counter.Increment();
            }
            if (reply) freeReplyObject(reply);
            // Increment write counter
            redisReply* incr_reply = (redisReply*)redisCommand(redis, "INCR stats:redis_writes");
            redis_requests_counter.Increment();
            if (!(incr_reply && incr_reply->type == REDIS_REPLY_INTEGER)) {
                redis_errors_counter.Increment();
            }
            if (incr_reply) freeReplyObject(incr_reply);
        } else {
            redis_errors_counter.Increment();
        }

        if (!redis_push_success) {
            client_errors_counter.Increment();
        }

        // Wait for response (simplified - in real implementation would poll Redis)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Send response
        // auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        //     std::chrono::system_clock::now().time_since_epoch()).count();
        // std::string response = "{\"message\": \"Processed by C++ DMZ Proxy\", \"request_id\": \"" + request_id + "\", \"language\": \"C++\", \"timestamp\": " + std::to_string(timestamp) + "}";

// Send response
        json::value response;
        response[U("message")] = json::value::string(U("Processed by C++ DMZ Proxy"));
        response[U("request_id")] = json::value::string(utility::conversions::to_string_t(request_id));
        response[U("language")] = json::value::string(U("C++"));
        response[U("timestamp")] = json::value::number(std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
        std::cout << "response" << response << std::endl;

        std::string response_json = utility::conversions::to_utf8string(response.serialize());
        bytes_sent_counter.Increment(response_json.size());
        mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n%s", response_json.c_str());
        return true;
    }
};

int main() {
    std::string redis_host = "valkey";
    int redis_port = 6379;

    redisContext* redis = redisConnect(redis_host.c_str(), redis_port);
    if (redis == NULL || redis->err) {
        std::cerr << "Redis connection error: " << (redis ? redis->errstr : "can't allocate redis context") << std::endl;
        return 1;
    }

    // Register signal handler for graceful shutdown
    std::signal(SIGTERM, signal_handler);
    std::signal(SIGINT, signal_handler);

    HealthHandler health_handler(redis);
    RequestHandler request_handler(redis);
    StatsHandler stats_handler(redis);

     const char *options[] = {
	  "listening_ports", "8888",
	  "num_threads", "32",
	  "enable_directory_listing", "no",
//      "max_connections", "1024",
      "request_timeout_ms", "30000",
	  0 };
	std::vector<std::string> cpp_options;
	for (int i = 0; i < (sizeof(options) / sizeof(options[0]) - 1); i++) {
		cpp_options.push_back(options[i]);
		std::cout << options[i];
		std::cout << std::endl;
	}

    // Start Prometheus exposer
    prometheus::Exposer exposer{"0.0.0.0:9090"};
    exposer.RegisterCollectable(registry);

    try {
        CivetServer server(cpp_options);
        server.addHandler("/health", &health_handler);
        server.addHandler("/stats", &stats_handler);
        server.addHandler("/", &request_handler);

        std::cout << "C++ DMZ Proxy listening on http://0.0.0.0:8888" << std::endl;
        std::cout << "Prometheus metrics available at http://0.0.0.0:9090/metrics" << std::endl;

        while (g_exit_flag == 0)
        {
            sleep(1);
        }
    }
    catch (CivetException& e)
    {
        std::cout << "CivetException:" << e.what() << std::endl;
    }

    redisFree(redis);

    return 0;
}
