#include "Subscriber.h"

Subscriber::Subscriber(grpc::ServerReaderWriter<MessegeToVtep, MacInfo>* stream)
    : stream_(stream)
{
    worker_thread_ = std::thread([this]() {
        MessegeToVtep msg;
        while (messages_.Pop(msg)) {
            if (!stream_->Write(msg)) {
                break;
            }
        }
    });
}

bool Subscriber::operator==(grpc::ServerReaderWriter<MessegeToVtep, MacInfo>* stream) {
    return stream_ == stream;
}

Subscriber::~Subscriber() {
    messages_.Abort();

    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
}

void Subscriber::Send(const MessegeToVtep& msg) {
    messages_.Push(msg);
}
