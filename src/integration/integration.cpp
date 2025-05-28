#include "integration.h"
#include <curl/curl.h>
#include <json/json.h>
#include <grpc++/grpc++.h>
#include <protobuf/service.grpc.pb.h>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <queue>
#include <condition_variable>

namespace core {
namespace integration {

// Реализация ExternalSystemConnector
class ExternalSystemConnector::Impl {
public:
    CURL* curl_handle;
    std::mutex curl_mutex;
    std::string base_url;
    std::string auth_token;
    
    // gRPC специфичные члены
    std::unique_ptr<grpc::Channel> grpc_channel;
    std::unique_ptr<Service::Stub> grpc_stub;
    
    // Очередь сообщений
    struct Message {
        std::string endpoint;
        std::string payload;
        std::function<void(const std::string&)> callback;
    };
    
    std::queue<Message> message_queue;
    std::mutex queue_mutex;
    std::condition_variable queue_cv;
    std::thread worker_thread;
    bool should_stop{false};
    
    Impl() {
        curl_global_init(CURL_GLOBAL_ALL);
        curl_handle = curl_easy_init();
    }
    
    ~Impl() {
        should_stop = true;
        queue_cv.notify_all();
        if (worker_thread.joinable()) {
            worker_thread.join();
        }
        curl_easy_cleanup(curl_handle);
        curl_global_cleanup();
    }
};

ExternalSystemConnector::ExternalSystemConnector(const std::string& base_url,
                                               const std::string& auth_token)
    : impl_(std::make_unique<Impl>()) {
    impl_->base_url = base_url;
    impl_->auth_token = auth_token;
    
    // Инициализация gRPC канала
    impl_->grpc_channel = grpc::CreateChannel(
        base_url + ":50051",
        grpc::InsecureChannelCredentials()
    );
    impl_->grpc_stub = Service::NewStub(impl_->grpc_channel);
    
    // Запуск рабочего потока
    impl_->worker_thread = std::thread([this]() {
        while (!impl_->should_stop) {
            std::unique_lock<std::mutex> lock(impl_->queue_mutex);
            impl_->queue_cv.wait(lock, [this]() {
                return !impl_->message_queue.empty() || impl_->should_stop;
            });
            
            if (impl_->should_stop) break;
            
            Message msg = std::move(impl_->message_queue.front());
            impl_->message_queue.pop();
            lock.unlock();
            
            process_message(msg);
        }
    });
}

ExternalSystemConnector::~ExternalSystemConnector() = default;

void ExternalSystemConnector::send_request(const std::string& endpoint,
                                         const std::string& payload,
                                         std::function<void(const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(impl_->queue_mutex);
    impl_->message_queue.push({endpoint, payload, callback});
    impl_->queue_cv.notify_one();
}

void ExternalSystemConnector::process_message(const Impl::Message& msg) {
    std::lock_guard<std::mutex> lock(impl_->curl_mutex);
    
    curl_easy_reset(impl_->curl_handle);
    
    // Установка URL
    std::string url = impl_->base_url + msg.endpoint;
    curl_easy_setopt(impl_->curl_handle, CURLOPT_URL, url.c_str());
    
    // Установка заголовков
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, 
        ("Authorization: Bearer " + impl_->auth_token).c_str());
    curl_easy_setopt(impl_->curl_handle, CURLOPT_HTTPHEADER, headers);
    
    // Установка данных запроса
    curl_easy_setopt(impl_->curl_handle, CURLOPT_POSTFIELDS, msg.payload.c_str());
    
    // Callback для получения ответа
    std::string response_data;
    curl_easy_setopt(impl_->curl_handle, CURLOPT_WRITEFUNCTION,
        [](void* contents, size_t size, size_t nmemb, std::string* userp) {
            size_t realsize = size * nmemb;
            userp->append((char*)contents, realsize);
            return realsize;
        });
    curl_easy_setopt(impl_->curl_handle, CURLOPT_WRITEDATA, &response_data);
    
    // Выполнение запроса
    CURLcode res = curl_easy_perform(impl_->curl_handle);
    curl_slist_free_all(headers);
    
    if (res != CURLE_OK) {
        msg.callback("Error: " + std::string(curl_easy_strerror(res)));
        return;
    }
    
    msg.callback(response_data);
}

std::string ExternalSystemConnector::send_grpc_request(const std::string& method,
                                                     const std::string& request) {
    grpc::ClientContext context;
    context.AddMetadata("authorization", "Bearer " + impl_->auth_token);
    
    Request grpc_request;
    grpc_request.set_data(request);
    
    Response grpc_response;
    grpc::Status status = impl_->grpc_stub->Process(&context, grpc_request, &grpc_response);
    
    if (!status.ok()) {
        throw std::runtime_error("gRPC request failed: " + status.error_message());
    }
    
    return grpc_response.data();
}

// Реализация DataFormatConverter
class DataFormatConverter::Impl {
public:
    Json::Value parse_json(const std::string& json_str) {
        Json::Value root;
        Json::Reader reader;
        if (!reader.parse(json_str, root)) {
            throw std::runtime_error("Failed to parse JSON: " + 
                                   reader.getFormattedErrorMessages());
        }
        return root;
    }
    
    std::string to_json(const Json::Value& value) {
        Json::FastWriter writer;
        return writer.write(value);
    }
};

DataFormatConverter::DataFormatConverter()
    : impl_(std::make_unique<Impl>()) {}

DataFormatConverter::~DataFormatConverter() = default;

std::string DataFormatConverter::convert_format(const std::string& data,
                                              const std::string& from_format,
                                              const std::string& to_format) {
    if (from_format == "json" && to_format == "json") {
        // Простая валидация JSON
        return impl_->to_json(impl_->parse_json(data));
    }
    
    // Здесь можно добавить конвертацию между другими форматами
    throw std::runtime_error("Unsupported format conversion: " + 
                           from_format + " to " + to_format);
}

} // namespace integration
} // namespace core 