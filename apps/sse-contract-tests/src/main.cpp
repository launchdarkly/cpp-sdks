#include "server.hpp"

#include <iostream>

int
main(int argc, char* argv[])
{
    try
    {
        const auto address = net::ip::make_address("0.0.0.0");
        unsigned short port = 8111;
        net::io_context ioc;

        server s{ioc.get_executor(), address, port};
        s.add_capability("headers");
        s.start();

        std::cout << "listening on " << address << ":" << port << std::endl;

        ioc.run();
    }
    catch(std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
