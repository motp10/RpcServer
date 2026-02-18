#include <grpcpp/grpcpp.h>
#include "lib/servises.h"
#include "proto/testproto.grpc.pb.h"
#include "SubscribersLib/ConnectionManager.h"

using grpc::ServerContext;
using grpc::Status;

class ChatServiceImpl final : public FdbServiceProto::Service {
    ConnectionManager connection_meneger_;
    FdbService& service_;

public:
    ChatServiceImpl(FdbService& service) : service_(service) {};

    Status Execute(grpc::ServerContext* context, grpc::ServerReaderWriter<MessegeToVtep, MacInfo>* stream) override {
        connection_meneger_.Subscribe(stream);
        MacInfo msg;
        MessegeToVtep response;
        
        while (stream->Read(&msg)) {
            response.set_ip_address(msg.ip_address());
            response.set_vni(msg.vni());
            response.set_mac_address(msg.mac_address());
            if (msg.operation() == "add") {
                auto res = service_.AddMac(msg.vni(), msg.mac_address(),  msg.ip_address(), 0);
                if (res == AddResult::Ignored) {
                    std::cout << "Mac already exists\n";
                    response.set_operation_result("Mac already exists");
                    stream->Write(response);
                } else {
                    response.set_operation_result("added");
                    connection_meneger_.Broadcast(response, stream);
                    std::cout << "Added: " << msg.mac_address() << std::endl;
                }
            } else if (msg.operation() == "delete") {
                auto res = service_.DeleteMac(msg.vni(), msg.mac_address(),  msg.ip_address(), 0);
                if (res == DeleteResult::Ignored) {
                    std::cout << "Mac does not exist\n";
                    response.set_operation_result("Mac does not exist");
                    stream->Write(response);
                } else {
                    response.set_operation_result("deleted");
                    connection_meneger_.Broadcast(response, stream);
                    std::cout << "Deleted: " << msg.mac_address() << std::endl;
                }
            } else if (msg.operation() == "get_vteps") {
                std::cout << "trying get vteps\n";
                auto vteps = service_.GetAllVteps(msg.vni());
                if (vteps.empty()) std::cout << "emty\n";
                for (auto vtep : vteps) {
                    std::cout << vtep << '\n';
                }
            }
        }

        connection_meneger_.Unsubscribe(stream);

        return Status::OK;
    }
};

int main() {
    std::string path = "./file.txt";
    VniRepository vni_repository;
    PersistenceRepository persistance(path);
    FdbService fdb_logic(vni_repository, persistance);

    std::string server_address("0.0.0.0:50051");
    ChatServiceImpl service_provider(fdb_logic);

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service_provider);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    server->Wait();

    return 0;
}