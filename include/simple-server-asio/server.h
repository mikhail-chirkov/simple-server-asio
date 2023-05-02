#pragma once
#include <map>
#include <queue>
#include <ranges>
#include <memory>
#include <coroutine>

#include <asio.hpp>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/util/delimited_message_util.h>
#include <asio/awaitable.hpp>
#include <ClientUpdate.pb.h>
#include <ServerUpdate.pb.h>

#include <simple-server-asio/network_participant.h>

using asio::ip::port_type;
using asio::ip::tcp;
using google::protobuf::MessageLite;

class Server
{
public:
    using participant_type = NetworkParticipant<::ClientUpdate, ::ServerUpdate>;

    Server(port_type port) : port_{port}, acceptor_{ctx_, tcp::endpoint{tcp::v4(), port_}}
    {
        // accept one client
        asio::co_spawn(ctx_, accept_connection(0), asio::detached);
        ctx_thread_ = std::thread{[&]()
                                  { ctx_.run(); }};
        is_running_ = true;
    }
    auto accept_connection(int32_t client_id) -> asio::awaitable<void>
    {
        std::cout << "waiting for clients\n";
        auto ec = asio::error_code{};
        auto socket = co_await acceptor_.async_accept(asio::redirect_error(asio::use_awaitable, ec));
        if (ec)
        {
            std::cout << "failed to accept the connection, retrying \n";
            asio::co_spawn(ctx_, accept_connection(client_id), asio::detached);
        } else {
            std::cout << "new client connected from port: " << socket.remote_endpoint().port() << "\n";
            participants_.emplace(client_id, std::make_unique<participant_type>(ctx_, std::move(socket)));
        }
    }

    void disconnect()
    {
        std::cout << "disconnecting the server\n";
        ctx_.stop();
        ctx_thread_.join();
    }
    
    void update_session()
    {
        // custom server logic
    }

    auto is_server_running() -> bool
    {
        return is_running_.load();
    }

    ~Server()
    {
        disconnect();
    }
    Server(const Server&) = delete;
    Server(Server&&) = default;
    Server& operator=(const Server&) = delete;
    Server& operator=(Server&&) = default;

private:
    asio::io_context ctx_;
    std::thread ctx_thread_;
    tcp::acceptor acceptor_;
    std::map<uint32_t, std::unique_ptr<participant_type>> participants_;
    std::atomic_bool is_running_;
    asio::ip::port_type port_;
    std::mutex mtx_;
};