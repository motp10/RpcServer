#include "ConnectionManager.h"

void ConnectionManager::Subscribe(StreamPtr stream) {
    std::lock_guard<std::mutex> lock(mux_);
    subscribers_.push_back(std::make_unique<Subscriber>(stream));
}

void ConnectionManager::Unsubscribe(grpc::ServerReaderWriter<MessegeToVtep, MacInfo>* stream) {
    std::lock_guard<std::mutex> lock(mux_);
    subscribers_.erase(
        std::remove_if(subscribers_.begin(), subscribers_.end(),
            [stream](const std::unique_ptr<Subscriber>& sub) {
                return *sub == stream;
            }),
        subscribers_.end()
    );
}

void ConnectionManager::Broadcast(const MessegeToVtep& msg, grpc::ServerReaderWriter<MessegeToVtep, MacInfo>* originator) {
    std::lock_guard<std::mutex> lock(mux_);
    for (auto& sub : subscribers_) {
        if (!(*sub == originator)) {
            sub->Send(msg);
        }
    }
}