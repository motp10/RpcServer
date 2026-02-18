#include "repositories.h"
#include <grpc/event_engine/memory_allocator.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <chrono>
#include <thread>

#pragma once

enum class Action { Add, Update, Block };

class FdbService {

public:
    FdbService(IVniRepository& r, PersistenceRepository& s);
    AddResult AddMac(uint32_t vni_id, MacAddress mac, VtepIp vtep, int64_t ts);
    DeleteResult DeleteMac(uint32_t vni_id, MacAddress mac, VtepIp vtep, int64_t ts);
    std::vector<VtepIp> GetAllVteps(VniId vni_id);

private:
    IVniRepository& repo_;
    IPersistenceRepository& storage_;
    mutable std::mutex mux;
};