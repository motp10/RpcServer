#include "repositories.h"

void MacRecord::Update(VtepIp new_vtep, int64_t ts, uint32_t flap_threshold_ms) {
    if (current_vtep != new_vtep) {
        if (ts - last_seen_ts < flap_threshold_ms) {
            flap_count++;
        } else {
            flap_count = 0;
        }
        previous_vtep = current_vtep;
        current_vtep = new_vtep;
    }
    last_seen_ts = ts;
}

VniContext::VniContext(VniId vni_id) : vni_id_(vni_id) {}

AddResult VniContext::AddAddress(MacAddress mac, VtepIp vtep, int64_t ts) {
    std::lock_guard<std::mutex> lk(mux_);
    if (fdb_table_.count(mac) == 0) {
        active_vteps_.push_back(vtep);
        fdb_table_[mac].Update(vtep, ts, 1000);
        return AddResult::Added;
    }

    auto record =fdb_table_[mac];

    if (record.current_vtep == vtep) {
        return AddResult::Ignored;
    }

    record.Update(vtep, ts, 1000);

    if (record.flap_count > 3) {
        record.status = RecordStatus::Unstable;
        return AddResult::FlappingDetected;
    }

    return AddResult::Updated;
}

DeleteResult VniContext::DeleteAddress(MacAddress mac, VtepIp vtep, int64_t ts) {
    std::lock_guard<std::mutex> lk(mux_);
    if (fdb_table_.count(mac) == 0) {
        return DeleteResult::Ignored;
    }
    fdb_table_.erase(mac);
    return DeleteResult::Deleted;
}

const std::vector<VtepIp>& VniContext::GetAllVteps() const { return active_vteps_; }

VniContext& VniRepository::GetOrCreateContext(uint32_t vni_id) {
    auto [it, _] = contexts_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(vni_id),
        std::forward_as_tuple(vni_id)
    );
    return it->second;
}

std::vector<VniId> VniRepository::GetAllVniIds() const {
    std::vector<VniId> ids;
    ids.reserve(contexts_.size());
    for (const auto& [id, _] : contexts_) {
        ids.push_back(id);
    }
    return ids;
}

bool VniRepository::Exists(VniId vni_id) const {
    return contexts_.count(vni_id) != 0;
}

PersistenceRepository::PersistenceRepository(const std::string& path) : journal_path_(path) {
    journal_file_.open(path, std::ios::binary | std::ios::app);
}

void PersistenceRepository::LogEvent(uint32_t vni, MacAddress mac, VtepIp vtep, int64_t ts, const std::string& command) {
    JournalEntry entry{vni, mac, vtep, ts, command};
    journal_file_.write(reinterpret_cast<const char*>(&entry), sizeof(entry));
    journal_file_.flush();
}

void PersistenceRepository::Restore(IVniRepository& repo) {
    std::ifstream is(journal_path_, std::ios::binary);
    if (!is.is_open()) return;

    JournalEntry entry;
    while (is.read(reinterpret_cast<char*>(&entry), sizeof(entry))) {
        VniContext& ctx = repo.GetOrCreateContext(entry.vni_id);
        ctx.AddAddress({entry.mac_addr}, {entry.vtep_ip}, entry.timestamp);
    }
    std::cout << "Восстановление завершено." << std::endl;
}

