#include "stdafx.h"
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>

namespace clustery {

void client(boost::asio::io_service &io_service, char const *hostname, int port, char const *node);
void server(boost::asio::io_service &io_service, int *ports, int num_ports);

int const PORT = 52900;
int       port = PORT;

void help()
{
    std::cout << "\nUsage: clustery [--client] [port]";
    std::cout << "\n\nDefault port is " << port;
}

}   // namespace clustery

namespace {

bool termination_requested = false;
boost::asio::io_service io_service;

#ifdef _MSC_VER
BOOL sigintHandler(DWORD sig_num)
#else
void sigintHandler(int sig_num)
#endif
{
    std::clog << "\nAbort signal received.";
    termination_requested = true;
    io_service.stop();
#ifdef _MSC_VER
    return TRUE;
#else
    /* Reset handler to catch SIGINT next time.
       Refer http://en.cppreference.com/w/c/program/signal */
    signal(SIGINT, sigintHandler);
#endif
}

}   // anonymous namespace

int main(int argc, char *argv[])
{
#ifdef _MSC_VER
    SetConsoleCtrlHandler(sigintHandler, TRUE);
#ifndef NDEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
#else
    signal(SIGINT, sigintHandler);
#endif

    try
    {
        bool client = false;
        char const *node = "local";
        for (int loop=1; loop<argc; ++loop)
        {
            if (strcmp(argv[loop], "--help") == 0)
            {
                clustery::help();
                return 0;
            }
            else if (strcmp(argv[loop], "--client") == 0)
                client = true;
            else if (strncmp(argv[loop], "--node=", 7) == 0)
                node = &argv[loop][7];
            else
                clustery::port = (unsigned short)std::atoi(argv[loop]);
        }

        
        boost::asio::io_service::work work(io_service);
        std::thread t = std::thread([](){ io_service.run(); });
        if (client)
        {
            clustery::client(io_service, "localhost", clustery::port, node);
            io_service.stop();
        }
        else
            clustery::server(io_service, &clustery::port, 1);

        t.join();
    }
    catch (std::exception const &e)
    {
        if (!termination_requested)
            std::cerr << "\n" << e.what();
    }

    std::cout << "\n\n";
	return 0;
}
