#include <grpcpp/support/sync_stream.h>
#include <iostream>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include <unistd.h>
#include "proto/testproto.pb.h" 
#include "proto/testproto.grpc.pb.h" 
#include "lib/servises.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

class FdbClient {
public:
    FdbClient(std::shared_ptr<Channel> channel, const std::string& hostname, uint32_t ip)
        : stub_(FdbServiceProto::NewStub(channel)),
          hostname_(hostname),
          ip_(ip){}

    void StartChat() {
        ClientContext context;
        std::shared_ptr<grpc::ClientReaderWriter<MacInfo, MessegeToVtep>> stream(
            stub_->Execute(&context));

        std::thread writer_thread([stream]() {
            MessegeToVtep server_msg;
            while (stream->Read(&server_msg)) {
                std::cout << "\n update: "
                          << server_msg.operation_result() << std::endl
                          << "VNI: " << server_msg.vni() 
                          << " MAC: " << server_msg.mac_address() << std::endl;
            }
            std::cout << "stream closed." << std::endl;
        });

        std::cout << "Mac VNI" << std::endl;
        uint32_t vni, mac;
        std::string command;
        while (std::cin >> command >> vni >> mac && (vni != 0 || mac != 0)) {
            MacInfo msg;
            msg.set_operation(command);
            msg.set_vni(vni);
            msg.set_mac_address(mac);
            msg.set_ip_address(ip_);
            if (!stream->Write(msg)) {
                std::cerr << "stream writing error" << std::endl;
                break;
            }
        }

        stream->WritesDone();
        writer_thread.join();
        Status status = stream->Finish();

        if (!status.ok()) {
            std::cerr << "RPC closed with error " << status.error_message() << std::endl;
        }
    }

private:
    std::string hostname_;
    uint32_t ip_;
    std::unique_ptr<FdbServiceProto::Stub> stub_;
};

int main(int argc, char** argv) {
    auto channel = grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());
    uint32_t ip = argc > 1 ? std::stoi(argv[1]) : 0;
    std::string hostname = argc > 2 ? argv[2] : "";
    FdbClient client(channel, hostname, ip);

    std::string command;
    std::cout << "FDB Console Client started." << std::endl;
    std::cout << "Commands: 'add' to send request, 'exit' to quit." << std::endl;

    client.StartChat();
    std::cout << "Goodbye!" << std::endl;
    return 0;
}