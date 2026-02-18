#include <vector>
#include <mutex>
#include <algorithm>
#include "proto/testproto.grpc.pb.h"
#include "Subscriber.h"

#pragma once

class ConnectionManager {
    using StreamPtr = grpc::ServerReaderWriter<MessegeToVtep, MacInfo>*;

private:
    std::mutex mux_;
    std::vector<std::unique_ptr<Subscriber>> subscribers_;

public:
    void Subscribe(StreamPtr stream);

    void Unsubscribe(grpc::ServerReaderWriter<MessegeToVtep, MacInfo>* stream);

    void Broadcast(const MessegeToVtep& msg, grpc::ServerReaderWriter<MessegeToVtep, MacInfo>* originator);
};