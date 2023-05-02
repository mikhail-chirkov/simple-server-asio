
set(PROTOBUF_MSGS 
    ${PROJECT_SOURCE_DIR}/proto/ClientUpdate.proto
    ${PROJECT_SOURCE_DIR}/proto/ServerUpdate.proto
)
add_executable(run_server
    ${PROTOBUF_MSGS}
    run_server.cpp
)

target_link_libraries(run_server asio::asio protobuf::protobuf)

target_include_directories(run_server PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
)

protobuf_generate(
    TARGET run_server
    IMPORT_DIRS "${PROJECT_SOURCE_DIR}/proto"
    PROTOC_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}"
)

add_executable(run_client
    ${PROTOBUF_MSGS}
    run_client.cpp
)

target_link_libraries(run_client asio::asio protobuf::protobuf)

target_include_directories(run_client PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
)

protobuf_generate(
    TARGET run_client
    IMPORT_DIRS "${PROJECT_SOURCE_DIR}/proto"
    PROTOC_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}"
)

