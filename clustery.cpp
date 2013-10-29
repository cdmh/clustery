#include "stdafx.h"
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>

namespace clustery {

#define PORT 52900

void client(char const *hostname, int port);
void server(int *ports, int num_ports);

int port = PORT;

void help()
{
    std::cout << "\nUsage: clustery [--client] [port]";
    std::cout << "\n\nDefault port is " << port;
}

}   // namespace clustery

int main(int argc, char *argv[])
{
    try
    {
        if (argc > 1)
        {
            if (strcmp(argv[1], "--help") == 0)
            {
                clustery::help();
                return 0;
            }
            else if (strcmp(argv[1], "--client") == 0)
            {
                if (argc > 2)
                    clustery::port = (unsigned short)std::atoi(argv[2]);

                clustery::client("localhost", clustery::port);
                return 0;
            }
            else
            {
                clustery::port = (unsigned short)std::atoi(argv[1]);
            }
        }

        clustery::server(&clustery::port, 1);
    }
    catch (std::exception const &e)
    {
        std::cerr << "\n" << e.what();
    }

    std::cout << "\n\n";
	return 0;
}
