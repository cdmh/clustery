#include "stdafx.h"
#include <cstdlib>
#include <deque>
#include <iostream>
#include <thread>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include "message.h"

using boost::asio::ip::tcp;

class comms_client
{
  public:
    comms_client(boost::asio::io_service &io_service, tcp::resolver::iterator endpoint_iterator)
      : io_service_(io_service),
        socket_(io_service)
    {
        do_connect(endpoint_iterator);
    }

    void write(const message &msg)
    {
        io_service_.post(
            [this, msg]() {
                bool write_in_progress = !write_msgs_.empty();
                write_msgs_.push_back(msg);
                if (!write_in_progress)
                    do_write();
            });
    }

    void close()
    {
        // queue a call to close the socket when the asio is idle
        io_service_.post([this]() { socket_.close(); });
    }

  private:
    void do_connect(tcp::resolver::iterator endpoint_iterator)
    {
        boost::asio::async_connect(
            socket_,
            endpoint_iterator,
            [this](boost::system::error_code ec, tcp::resolver::iterator)
            {
                if (!ec)
                    do_read_header();
            });
    }

    void do_read_header() // !!!duplicated function
    {
        boost::asio::async_read(
            socket_,
            read_msg_.header_buffer(),
            [this](boost::system::error_code ec, std::size_t length)
            {
                if (!ec)
                    do_read_body();
                else
                    socket_.close();
            });
    }

    void do_read_body()   //!!! duplicate function
    {
        boost::asio::async_read(
            socket_,
            read_msg_.body_buffer(),
            [this](boost::system::error_code ec, std::size_t /*length*/)
            {
                if (!ec)
                {
                    std::cout.write(read_msg_.body(), read_msg_.body_length());
                    std::cout << "\n";
                    do_read_header();
                }
                else
                    socket_.close();
            });
    }

    void do_write()       //!!! duplicate function
    {
        boost::asio::async_write(
            socket_, write_msgs_.front().header_buffer(),
            [this](boost::system::error_code ec, std::size_t /*length*/)
            {
                if (!ec)
                    do_body_write();
                else
                    socket_.close();
            });
    }

  private:
    void do_body_write() //!!! duplicate function
    {
        boost::asio::async_write(
            socket_, write_msgs_.front().body_buffer(),
            [this](boost::system::error_code ec, std::size_t /*length*/)
            {
                if (!ec)
                {
                    write_msgs_.pop_front();
                    if (!write_msgs_.empty())
                        do_write();
                }
                else
                    socket_.close();
            });
    }

  private:
    boost::asio::io_service &io_service_;
    tcp::socket              socket_;
    message                  read_msg_;
    message_queue_t          write_msgs_;
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
