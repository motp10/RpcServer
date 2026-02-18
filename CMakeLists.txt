cmake_minimum_required(VERSION 3.15)
project(ProtoTestProject)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

find_program(gRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)

add_library(protolib proto/testproto.proto)

target_link_libraries(protolib
    gRPC::grpc++
)

target_include_directories(protolib PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

protobuf_generate(
    TARGET protolib
    LANGUAGE cpp
)

protobuf_generate(
    TARGET protolib
    LANGUAGE grpc
    GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
    PLUGIN "protoc-gen-grpc=${gRPC_CPP_PLUGIN_EXECUTABLE}"
)

add_library(app_common
    lib/repositories.cpp
    lib/services.cpp
)

add_library(subscribers_lib
    SubscribersLib/ConnectionManager.cpp
    SubscribersLib/Subscriber.cpp
)

target_link_libraries(subscribers_lib
    protolib
)

target_link_libraries(app_common
    subscribers_lib
    gRPC::grpc++
)

target_include_directories(app_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_include_directories(subscribers_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(my_app server.cpp)
add_executable(client_app1 client.cpp)
add_executable(client_app2 client.cpp)

target_link_libraries(my_app app_common)
target_link_libraries(client_app1 app_common)
target_link_libraries(client_app2 app_common)

add_dependencies(app_common protolib)
add_dependencies(my_app app_common)
add_dependencies(client_app1 app_common)
add_dependencies(client_app2 app_common)