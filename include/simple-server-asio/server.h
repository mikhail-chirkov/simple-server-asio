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

    Server(port_type port) : port_{port}, acceptor_{ctx_, tcp::endpoint{tcp::v4(), port}}
    {
        // accept one client
        asio::co_spawn(ctx_, accept_connection(7), asio::detached);
        // run context in the backround
        ctx_thread_ = std::thread{[&]()
                                  { 
                                    auto wg = asio::make_work_guard(ctx_);
                                    ctx_.run(); }};
        is_running_ = true;
    }
    
    auto accept_connection(int32_t client_id) -> asio::awaitable<void>
    {
        std::cout << "waiting for a client with id " << client_id << '\n';
        auto ec = asio::error_code{};
        auto socket = co_await acceptor_.async_accept(asio::redirect_error(asio::use_awaitable, ec));
        if (ec)
        {
            std::cout << "failed to accept the connection, retrying \n";
            asio::co_spawn(ctx_, accept_connection(client_id), asio::detached);
        } else {
            std::cout << "new client connected from port: " << socket.remote_endpoint().port() << "\n";
            participants_.emplace(client_id, std::make_unique<participant_type>(ctx_, std::move(socket)));
            // send id to the client
            auto msg = ServerUpdate{};
            msg.set_id(client_id);
            participants_.at(client_id)->write_to_output_queue(std::move(msg));
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
        // read pending messages
        for(const auto& client : participants_ | std::ranges::views::values){
            if(auto msg = client->read_from_input_queue()){
                // custom server logic
                //msg
            }
        }
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

    std::atomic_bool is_running_;

    tcp::acceptor acceptor_;
    asio::ip::port_type port_;
    std::map<uint32_t, std::unique_ptr<participant_type>> participants_;
};