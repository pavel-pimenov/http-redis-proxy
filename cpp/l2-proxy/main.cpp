#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <cpprest/base_uri.h>
#include <hiredis/hiredis.h>
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <random>

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;

class DMZProxy {
private:
    http_listener listener;
    redisContext* redis;
    
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

public:
    DMZProxy(const std::string& url, const std::string& redis_host, int redis_port) 
        : listener(url) {
        
        redis = redisConnect(redis_host.c_str(), redis_port);
        if (redis == NULL || redis->err) {
            std::cerr << "Redis connection error: " << (redis ? redis->errstr : "can't allocate redis context") << std::endl;
        }
    }

    ~DMZProxy() {
        if (redis) {
            redisFree(redis);
        }
    }

    void handle_health(http_request request) {
        if (redis && redis->err) {
            request.reply(status_codes::ServiceUnavailable, "Redis unavailable");
            return;
        }
        
        redisReply* reply = (redisReply*)redisCommand(redis, "PING");
        if (reply && reply->type == REDIS_REPLY_STATUS) {
            request.reply(status_codes::OK, "OK");
        } else {
            request.reply(status_codes::ServiceUnavailable, "Redis unavailable");
        }
        if (reply) freeReplyObject(reply);
    }

    void handle_request(http_request request) {
        auto request_id = generate_uuid();
        
        // Prepare request data for Redis
        json::value request_data;
        request_data[U("id")] = json::value::string(utility::conversions::to_string_t(request_id));
        request_data[U("method")] = json::value::string(utility::conversions::to_string_t(request.method()));
        request_data[U("path")] = json::value::string(request.relative_uri().path());
        
        // Push to Redis queue
        if (redis && !redis->err) {
            auto request_str = utility::conversions::to_utf8string(request_data.serialize());
            redisReply* reply = (redisReply*)redisCommand(redis, "RPUSH http:requests %s", request_str.c_str());
            if (reply) freeReplyObject(reply);
        }

        // Wait for response (simplified - in real implementation would poll Redis)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Send response
        json::value response;
        response[U("message")] = json::value::string(U("Processed by C++ DMZ Proxy"));
        response[U("request_id")] = json::value::string(utility::conversions::to_string_t(request_id));
        response[U("language")] = json::value::string(U("C++"));
        response[U("timestamp")] = json::value::number(std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
        
        request.reply(status_codes::OK, response);
    }

    void init() {
        listener.support(methods::GET, [this](http_request request) {
            if (request.relative_uri().path() == U("/health")) {
                handle_health(request);
            } else {
                handle_request(request);
            }
        });
        
        listener.support(methods::POST, [this](http_request request) {
            handle_request(request);
        });
    }

    pplx::task<void> open() { return listener.open(); }
    pplx::task<void> close() { return listener.close(); }
};

int main() {
    std::string redis_host = "valkey";
    int redis_port = 6379;
    
    DMZProxy proxy("http://0.0.0.0:8080", redis_host, redis_port);
    proxy.init();
    
    try {
        proxy.open().wait();
        std::cout << "C++ DMZ Proxy listening on http://0.0.0.0:8080" << std::endl;
        std::cout << "Press Enter to exit..." << std::endl;
        
        std::string line;
        std::getline(std::cin, line);
        
        proxy.close().wait();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    
    return 0;
}
