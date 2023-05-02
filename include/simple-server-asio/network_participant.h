#pragma once
#include <mutex>
#include <queue>
#include <coroutine>

#include <asio.hpp>
#include <asio/awaitable.hpp>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/util/delimited_message_util.h>

using asio::ip::tcp;
using google::protobuf::MessageLite;
using google::protobuf::io::IstreamInputStream;
using google::protobuf::util::ParseDelimitedFromZeroCopyStream;
using google::protobuf::util::SerializeDelimitedToOstream;

template <typename T>
concept ProtoMsgConcept = std::is_base_of_v<MessageLite, T>;

template <ProtoMsgConcept InputMsgT, ProtoMsgConcept OutputMsgT>
class NetworkParticipant
{
public:
    NetworkParticipant(asio::io_context &ctx, tcp::socket &&socket) : socket_{std::move(socket)}, ctx_{ctx}, os_{&output_buffer_}, is_{&input_buffer_}, proto_is_{&is_}
    {
        asio::co_spawn(ctx_, read_async(), asio::detached);
        asio::co_spawn(ctx_, write_async(), asio::detached);
    }

    void write_to_output_queue(const OutputMsgT &out_msg)
    {
        const auto l = std::scoped_lock{output_mtx_};
        output_message_queue_.push(out_msg);
    }

    auto read_from_input_queue() -> InputMsgT
    {
        wait_for_message();
        const auto l = std::scoped_lock{input_mtx_};
        auto msg = std::move(input_message_queue_.front());
        input_message_queue_.pop();
        return msg;
    }

    ~NetworkParticipant()
    {
        socket_.close();
    }
    NetworkParticipant(const NetworkParticipant&) = delete;
    NetworkParticipant(NetworkParticipant&&) = default;
    NetworkParticipant& operator=(const NetworkParticipant&) = delete;
    NetworkParticipant& operator=(NetworkParticipant&&) = default;

private:

    asio::awaitable<void> read_async()
    {
        co_await asio::post(ctx_, asio::use_awaitable);
        const auto l = std::scoped_lock{input_mtx_};

        if (socket_.is_open())
        {
            auto ec = asio::error_code{};
            auto temp = input_buffer_.prepare(128);
            
            auto bytes_read = co_await socket_.async_read_some(temp, asio::redirect_error(asio::use_awaitable, ec));
            if (ec and ec != asio::error::eof)
            {
                std::cout << ec.message();
            }
            auto msg = InputMsgT{};
            ParseDelimitedFromZeroCopyStream(&msg, &proto_is_, nullptr);
            // input_buffer_.consume(bytes_read);
            input_message_queue_.push(msg);
        }
        asio::co_spawn(ctx_, read_async(), asio::detached);
    }

    asio::awaitable<void> write_async()
    {
        co_await asio::post(ctx_, asio::use_awaitable);
        const auto l = std::scoped_lock{output_mtx_};
        if (not output_message_queue_.empty() and socket_.is_open())
        {
            while (not output_message_queue_.empty())
            {
                auto &msg = output_message_queue_.front();
                SerializeDelimitedToOstream(msg, &os_);
                output_message_queue_.pop();
            }
            auto ec = asio::error_code{};
            auto bytes_sent = co_await asio::async_write(socket_, output_buffer_, asio::redirect_error(asio::use_awaitable, ec));
            if (ec and ec != asio::error::eof)
            {
                std::cout << ec.message();
            }
            output_buffer_.consume(bytes_sent);
        }
        asio::co_spawn(ctx_, write_async(), asio::detached);
    }

    void wait_for_message()
    {
        auto ul = std::unique_lock{input_mtx_};
        cv_.wait(ul, [&]
                 {return not input_message_queue_.empty(); });
    }

    int32_t id_;
    tcp::socket socket_;

    asio::io_context &ctx_;
    std::queue<OutputMsgT> output_message_queue_;
    asio::streambuf output_buffer_;
    std::ostream os_;
    std::mutex output_mtx_;

    std::queue<InputMsgT> input_message_queue_;
    asio::streambuf input_buffer_;
    std::istream is_;
    IstreamInputStream proto_is_;
    std::mutex input_mtx_;

    std::condition_variable cv_;
};