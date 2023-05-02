#include "simple-server-asio/server.h"
#include <thread>

int main(int argc, char const *argv[])
{
    auto server = Server{60000};

    // run server loop
    while (server.is_server_running())
    {
        ::std::this_thread::sleep_for(std::chrono::milliseconds(100));
        server.update_session();
    }
}
