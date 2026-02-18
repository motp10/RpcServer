#include <fstream>
#include <iostream>
#include <cinttypes>
#include <thread>
#include <vector>
#include <mutex>
#include <unordered_map>

#pragma once

using MacAddress = uint64_t;

using VniId = int;

using VtepIp = uint32_t;

enum class RecordStatus : uint8_t {
    Active = 1,
    Unstable = 2,
    Blocked = 3
};

enum class AddResult { 
    Ignored = 0,
    Added = 1, 
    Updated = 2, 
    FlappingDetected = 3
};

enum class DeleteResult { 
    Ignored = 0,
    Deleted = 1 
};

struct MacRecord {
    VtepIp current_vtep;
    int64_t last_seen_ts;
    RecordStatus status = RecordStatus::Active;
    
    uint8_t flap_count = 0;
    VtepIp previous_vtep;

    void Update(VtepIp new_vtep, int64_t ts, uint32_t flap_threshold_ms);
};

class VniContext {
public:
    VniContext(VniId vni_id);

    AddResult AddAddress(MacAddress mac, VtepIp vtep, int64_t ts);

    DeleteResult DeleteAddress(MacAddress mac, VtepIp vtep, int64_t ts);

    const std::vector<VtepIp>& GetAllVteps() const;

private:
    std::mutex mux_;
    VniId vni_id_;
    std::unordered_map<MacAddress, MacRecord> fdb_table_;
    std::vector<VtepIp> active_vteps_;
};

class IVniRepository {
public:
    virtual ~IVniRepository() = default;
    virtual VniContext& GetOrCreateContext(uint32_t vni_id) = 0;
    virtual std::vector<VniId> GetAllVniIds() const = 0;
    virtual bool Exists(VniId vni_id) const = 0;
};

class VniRepository : public IVniRepository {
private:
    std::unordered_map<VniId, VniContext> contexts_;

public:
    VniContext& GetOrCreateContext(uint32_t vni_id) override;

    std::vector<VniId> GetAllVniIds() const override;

    bool Exists(VniId vni_id) const override;
};

struct JournalEntry {
    uint32_t vni_id;
    uint64_t mac_addr;
    uint32_t vtep_ip;
    int64_t timestamp;
    std::string command;
};

class IPersistenceRepository {
public:
    virtual ~IPersistenceRepository() = default;
    
    virtual void LogEvent(uint32_t vni, MacAddress mac, VtepIp vtep, int64_t ts, const std::string& command) = 0;
    
    virtual void Restore(IVniRepository& repo) = 0;
};

class PersistenceRepository: public IPersistenceRepository{
private:
    std::string journal_path_;
    std::ofstream journal_file_;

public:
    explicit PersistenceRepository(const std::string& path);

    void LogEvent(uint32_t vni, MacAddress mac, VtepIp vtep, int64_t ts, const std::string& command);

    void Restore(IVniRepository& repo);
};

