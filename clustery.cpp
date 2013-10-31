#include "stdafx.h"
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include "comms_client.h"
#include "n3588.h"

namespace clustery {

void server(boost::asio::io_service &io_service, int *ports, int num_ports);

#define STRINGIFY(a) STRINGIFY_(a)
#define STRINGIFY_(a) #a

#define PORT 52900
int port = PORT;

void help()
{
    std::cerr << "\nUsage: clustery [--peer-node=...] [--peer-port=...][--node=...] [--port=port]";
    std::cerr << "\n\nDefault port is " << PORT;
}

}   // namespace clustery

namespace {

bool termination_requested = false;
boost::asio::io_service io_service;

#ifdef _MSC_VER
BOOL __stdcall sigintHandler(DWORD sig_num)
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
        char hostname[256];
        gethostname(hostname, sizeof(hostname));

        bool        show_help = false;
        char const *node      = hostname;
        char const *peer_node = nullptr;
        int         peer_port = PORT;
        for (int loop=1; loop<argc; ++loop)
        {
            if (strcmp(argv[loop], "--help") == 0)
                clustery::help();
            else if (strncmp(argv[loop], "--node=", 7) == 0)
                node = &argv[loop][7];
            else if (strncmp(argv[loop], "--port=", 7) == 0)
            {
                clustery::port = std::atoi(&argv[loop][7]);
                if (boost::lexical_cast<std::string>(clustery::port) != &argv[loop][7])
                    show_help = true;
            }
            else if (strncmp(argv[loop], "--peer-node=", 12) == 0)
                peer_node = &argv[loop][12];
            else if (strncmp(argv[loop], "--peer-port=", 12) == 0)
            {
                peer_port = std::atoi(&argv[loop][12]);
                if (boost::lexical_cast<std::string>(peer_port) != &argv[loop][12])
                    show_help = true;
            }
            else
                show_help = true;
        }

        std::cout << "\nClustery\n========\n";
        std::cout << "\nThis node (--node)     : " << node;
        std::cout << "\nThis port (--port)     : " << clustery::port;
        if (peer_node)
        {
            std::cout << "\nPeer node (--peer-node): " << peer_node;
            std::cout << "\nPeer port (--peer-port): " << peer_port;
        }
        std::cout << "\n";

        if (show_help)
        {
            clustery::help();
            return 0;
        }

        boost::asio::io_service::work work(io_service);
        std::thread t = std::thread([](){ io_service.run(); });
        clustery::server(io_service, &clustery::port, 1);

        std::unique_ptr<clustery::comms_client> client;
        if (peer_node)
            client = n3588::make_unique<clustery::comms_client>(node, io_service, hostname, peer_port);
        else
            std::clog << "\nStarting a new cluster.";

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
