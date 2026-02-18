#include <queue>
#include "build/proto/testproto.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include "ThreadSafeQueue.h"
#include <thread>

#pragma once

class Subscriber {
private:
    grpc::ServerReaderWriter<MessegeToVtep, MacInfo>* stream_;
    ThreadSafeQueue<MessegeToVtep> messages_;
    std::thread worker_thread_;

public:
    Subscriber(grpc::ServerReaderWriter<MessegeToVtep, MacInfo>* stream);

    bool operator==(grpc::ServerReaderWriter<MessegeToVtep, MacInfo>* stream);

    ~Subscriber();

    void Send(const MessegeToVtep& msg);

    Subscriber(const Subscriber&) = delete;
    Subscriber& operator=(const Subscriber&) = delete;
};