#include <microhttpd.h>
#include <jsoncpp/json/json.h>
#include <iostream>
#include <string>
#include <chrono>

class L2Server {
private:
    struct MHD_Daemon* daemon;
    
    static MHD_Result answer_to_connection(void* cls, struct MHD_Connection* connection,
                                   const char* url, const char* method,
                                   const char* version, const char* upload_data,
                                   size_t* upload_data_size, void** con_cls) {
        
        std::string response_json;
        Json::Value response;
        
        if (std::string(url) == "/health" && std::string(method) == "GET") {
            auto response_ptr = MHD_create_response_from_buffer(2, (void*)"OK", MHD_RESPMEM_PERSISTENT);
            MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response_ptr);
            MHD_destroy_response(response_ptr);
            return ret;
        }
        else if (std::string(url) == "/" && std::string(method) == "GET") {
            response["message"] = "Hello from C++ L2 Server!";
            response["language"] = "C++";
            response["timestamp"] = (Json::Int64)std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            response["server"] = "C++ L2 Server";
            response["status"] = "operational";
        }
        else if (std::string(url) == "/api/info" && std::string(method) == "GET") {
            response["server"] = "C++ L2 Server";
            response["status"] = "operational";
            response["framework"] = "libmicrohttpd";
            response["performance"] = "high";
        }
        else {
            response["error"] = "Endpoint not found";
            response["path"] = url;
            
            Json::StreamWriterBuilder writer;
            response_json = Json::writeString(writer, response);
            
            auto response_ptr = MHD_create_response_from_buffer(response_json.length(),
                                                              (void*)response_json.c_str(),
                                                              MHD_RESPMEM_MUST_COPY);
            MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response_ptr);
            MHD_destroy_response(response_ptr);
            return ret;
        }
        
        Json::StreamWriterBuilder writer;
        response_json = Json::writeString(writer, response);
        
        auto response_ptr = MHD_create_response_from_buffer(response_json.length(), 
                                                          (void*)response_json.c_str(), 
                                                          MHD_RESPMEM_MUST_COPY);
        MHD_add_response_header(response_ptr, "Content-Type", "application/json");
        
        MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response_ptr);
        MHD_destroy_response(response_ptr);
        return ret;
    }

public:
    L2Server(int port) {
        daemon = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD, port, NULL, NULL,
                                 &answer_to_connection, NULL, MHD_OPTION_END);
        if (!daemon) {
            std::cerr << "Failed to start L2 server on port " << port << std::endl;
            exit(1);
        }
    }
    
    ~L2Server() {
        if (daemon) {
            MHD_stop_daemon(daemon);
        }
    }
    
    void wait() {
        std::cout << "C++ L2 Server running on port 3000" << std::endl;
        std::cout << "Press Enter to stop..." << std::endl;
        std::cin.get();
    }
};

int main() {
    L2Server server(3000);
    server.wait();
    return 0;
}
