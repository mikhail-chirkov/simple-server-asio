#include <simple-server-asio/client.h>

int main(int argc, char const *argv[])
{
    auto c = Client{};
    c.connect("127.0.0.1", 60000);
    return 0;
}
