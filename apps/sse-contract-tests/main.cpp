#include "server.hpp"

int
main(int argc, char* argv[])
{
    try
    {
        auto address = net::ip::make_address("0.0.0.0");
        unsigned short port = 8123;
        net::io_context ioc;

        server s{ioc.get_executor(), std::move(address), port};
        s.add_capability("headers");

        s.start_accepting();
        ioc.run();
    }
    catch(std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
