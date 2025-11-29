#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <random>
#include <string>
#include <ctime>
#include <unistd.h>

#include "CivetServer.h"
#include <hiredis/hiredis.h>

#include <cpprest/json.h>

static int g_exit_flag = 0;

class HealthHandler : public CivetHandler {
private:
    redisContext* redis;

public:
    HealthHandler(redisContext* r) : redis(r) {}

    bool handleGet(CivetServer *server, struct mg_connection *conn) {
        if (redis && redis->err) {
            mg_printf(conn, "HTTP/1.1 503 Service Unavailable\r\nContent-Type: text/plain\r\n\r\nRedis unavailable");
            return true;
        }

        redisReply* reply = (redisReply*)redisCommand(redis, "PING");
        if (reply && reply->type == REDIS_REPLY_STATUS) {
            mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nOK");
        } else {
            mg_printf(conn, "HTTP/1.1 503 Service Unavailable\r\nContent-Type: text/plain\r\n\r\nRedis unavailable");
        }
        if (reply) freeReplyObject(reply);
        return true;
    }
};

class RequestHandler : public CivetHandler {
private:
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
    RequestHandler(redisContext* r) : redis(r) {}

    bool handleGet(CivetServer *server, struct mg_connection *conn) {
        return handle_request(server, conn, "GET");
    }

    bool handlePost(CivetServer *server, struct mg_connection *conn) {
        return handle_request(server, conn, "POST");
    }

    bool handle_request(CivetServer *server, struct mg_connection *conn, const std::string& method) {
        const struct mg_request_info *req_info = mg_get_request_info(conn);
        std::string path = req_info->request_uri ? req_info->request_uri : "/";

        auto request_id = generate_uuid();

        // Prepare request data for Redis
        // std::string request_data = "{\"id\": \"" + request_id + "\", \"method\": \"" + method + "\", \"path\": \"" + path + "\"}";

            // Prepare request data for Redis
        json::value request_data;
        request_data[U("id")] = json::value::string(utility::conversions::to_string_t(request_id));
        request_data[U("method")] = json::value::string(utility::conversions::to_string_t(request.method()));
        request_data[U("path")] = json::value::string(request.relative_uri().path());
        std::cout << "request_data: " << request_data << std::endl;

        // Push to Redis queue
        if (redis && !redis->err) {
            redisReply* reply = (redisReply*)redisCommand(redis, "RPUSH http:requests %s", request_data.c_str());
            if (reply) freeReplyObject(reply);
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
        
        mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n%s", response.c_str());
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

    HealthHandler health_handler(redis);
    RequestHandler request_handler(redis);

     const char *options[] = {
	  "listening_ports", "8888",
	  "num_threads", "10",
	  "enable_directory_listing", "no",
	  0 };
	std::vector<std::string> cpp_options;
	for (int i = 0; i < (sizeof(options) / sizeof(options[0]) - 1); i++) {
		cpp_options.push_back(options[i]);
		std::cout << options[i];
		std::cout << std::endl;
	}

    try {
        CivetServer server(cpp_options);
        // server.addHandler("/health", &health_handler);
        server.addHandler("/", &request_handler);

        std::cout << "C++ DMZ Proxy listening on http://0.0.0.0:8888" << std::endl;

        while (g_exit_flag == 0)
        {
            sleep(1);
        }
    }
    catch (CivetException& e)
    {
        std::cout << "CivetException:" << e.what() << std::endl;
    }

//   std::cout << "Press Enter to exit..." << std::endl;

//    std::string line;
//    std::getline(std::cin, line);

    // Server will stop when going out of scope
    redisFree(redis);

    return 0;
}
