#include <hiredis/hiredis.h>
#include <curl/curl.h>
#include <jsoncpp/json/json.h>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>

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

    std::string call_l2_server(const std::string& path) {
        std::string url = l2_server_url + path;
        std::string response_string;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            return "{\"error\": \"Failed to call L2 server: " + std::string(curl_easy_strerror(res)) + "\"}";
        }

        return response_string;
    }

    void process_request(const std::string& request_json) {
        Json::Value request_data;
        Json::Reader reader;
        
        if (!reader.parse(request_json, request_data)) {
            std::cerr << "Failed to parse JSON request" << std::endl;
            return;
        }

        std::string request_id = request_data["id"].asString();
        std::string path = request_data["path"].asString();

        std::cout << "Processing request: " << request_id << " path: " << path << std::endl;

        // Call L2 server
        std::string l2_response = call_l2_server(path);

        // Prepare response for Redis
        Json::Value response_data;
        response_data["status_code"] = 200;
        response_data["headers"]["Content-Type"] = "application/json";
        
        Json::Value body;
        body["message"] = "Processed by C++ L2 Worker";
        body["language"] = "C++";
        body["request_id"] = request_id;
        body["l2_response"] = l2_response;
        body["timestamp"] = (Json::Int64)std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        response_data["body"] = body;

        // Store response in Redis
        Json::StreamWriterBuilder writer;
        std::string response_str = Json::writeString(writer, response_data);
        
        redisReply* reply = (redisReply*)redisCommand(redis, "SETEX http:response:%s 60 %s", 
                                                     request_id.c_str(), response_str.c_str());
        if (reply) freeReplyObject(reply);
    }

    void run() {
        std::cout << "C++ L2 Worker started. Waiting for requests..." << std::endl;

        while (true) {
            redisReply* reply = (redisReply*)redisCommand(redis, "BLPOP http:requests 10");
            
            if (reply && reply->type == REDIS_REPLY_ARRAY && reply->elements == 2) {
                std::string request_json = reply->element[1]->str;
                process_request(request_json);
            }
            
            if (reply) freeReplyObject(reply);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
};

int main() {
    std::string redis_host = "valkey";
    int redis_port = 6379;
    std::string l2_server_url = "http://l2-server:3000";

    L2Worker worker(redis_host, redis_port, l2_server_url);
    worker.run();

    return 0;
}
