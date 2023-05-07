#pragma once
#include <memory>

#include <asio.hpp>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/util/delimited_message_util.h>
#include <ClientUpdate.pb.h>
#include <ServerUpdate.pb.h>

#include <simple-server-asio/network_participant.h>

using asio::ip::tcp;

class Client
{
public:
    using participant_type = NetworkParticipant<::ServerUpdate, ::ClientUpdate>;

    Client()
    {
        ctx_thread_ = std::thread{[&]()
                                  { 
                                    auto wg = asio::make_work_guard(ctx_);
                                    ctx_.run(); }};
    };

    void connect(const std::string &ip, asio::ip::port_type port)
    {
        auto socket = tcp::socket{ctx_};
        auto ep = tcp::endpoint{tcp::v4(), port};
        socket.connect(ep);
        std::cout << "connected to the server on port: " << port << "\n";
        participant_ = std::make_unique<participant_type>(ctx_, std::move(socket));
        // wait for id from the server
        auto msg = participant_->wait_and_read_from_input_queue();
        id_ = msg.id();
        std::cout << "Assigned client id is " << id_ << "\n";
    }

    void disconnect()
    {
        ctx_.stop();
        ctx_thread_.join();
    }
    ~Client()
    {
        disconnect();
    };
    
    Client(const Client&) = delete;
    Client(Client&&) = default;
    Client& operator=(const Client&) = delete;
    Client& operator=(Client&&) = default;

private:
    asio::io_context ctx_;
    std::thread ctx_thread_;

    std::unique_ptr<participant_type> participant_;
    asio::ip::port_type port_;
    uint32_t id_;
};