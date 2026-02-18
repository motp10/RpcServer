#include "servises.h"

FdbService::FdbService(IVniRepository& r, PersistenceRepository& s) 
    : repo_(r), storage_(s) {}

AddResult FdbService::AddMac(uint32_t vni_id, MacAddress mac, VtepIp vtep, int64_t ts) {
    std::lock_guard<std::mutex> lock(mux);
    storage_.LogEvent(vni_id, mac, vtep, ts, "add");

    VniContext& ctx = repo_.GetOrCreateContext(vni_id);
    auto res = ctx.AddAddress(mac,vtep, ts);
    return res;
}   

DeleteResult FdbService::DeleteMac(uint32_t vni_id, MacAddress mac, VtepIp vtep, int64_t ts) {
    std::lock_guard<std::mutex> lock(mux);
    storage_.LogEvent(vni_id, mac, vtep, ts, "delete");

    VniContext& ctx = repo_.GetOrCreateContext(vni_id);

    return ctx.DeleteAddress(mac, vtep, ts);
}

std::vector<VtepIp> FdbService::GetAllVteps(VniId vni_id) {
    std::lock_guard<std::mutex> lock(mux);
    VniContext& cntx = repo_.GetOrCreateContext(vni_id);
    return cntx.GetAllVteps();
}