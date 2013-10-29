#include "stdafx.h"
#include <cstdlib>
#include <deque>
#include <iostream>
#include <thread>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include "message.h"

class comms_client : public comms
{
  public:
    comms_client(boost::asio::io_service &io_service, tcp::resolver::iterator endpoint_iterator)
      : comms(tcp::socket(io_service)),
        io_service_(io_service)
    {
        connect(endpoint_iterator);
    }

    void close()
    {
        // queue a call to close the socket when the asio is idle
        io_service_.post([this]() { socket_.close(); });
    }

    void write(message const &msg)
    {
        io_service_.post(
            [this, msg]() {
                bool write_in_progress = !write_msgs_.empty();
                write_msgs_.push_back(msg);
                if (!write_in_progress)
                    write();
            });
    }

  private:
    void connect(tcp::resolver::iterator endpoint_iterator)
    {
        boost::asio::async_connect(
            socket_,
            endpoint_iterator,
            [this](boost::system::error_code ec, tcp::resolver::iterator)
            {
                if (!ec)
                    read();
            });
    }

    void read()
    {
        perform_read(
            [this](){
                std::cout.write(read_msg_.body(), read_msg_.body_length());
                std::cout << "\n";
                read();
            },
            [this]() { socket_.close(); });
    }

    void write()
    {
        perform_write(
            [this]() { write(); },
            [this]() { socket_.close(); });
    }

  private:
    boost::asio::io_service &io_service_;
};


void client(char const *hostname, int port)
{
    std::cout << "\nClustery client\n===============\nRunning ...\n\n";
    try
    {
        boost::asio::io_service io_service;

        auto endpoint_iterator = tcp::resolver(io_service).resolve({ hostname, boost::lexical_cast<std::string>(port).c_str() });
        comms_client c(io_service, endpoint_iterator);

        std::thread t([&io_service](){ io_service.run(); });

        std::string line;
        while (getline(std::cin, line))
        {
            message msg(line);
            c.write(msg);
        }

        c.close();
        t.join();
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}
