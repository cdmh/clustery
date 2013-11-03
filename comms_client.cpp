#include "stdafx.h"
#include "comms_client.h"

namespace clustery {

comms_client::comms_client(
    char const              *hostname,
    char const              *nodename,
    unsigned short           port,
    boost::asio::io_service &io_service,
    char const              *peer_node,
    unsigned short           peer_port)
  : comms(tcp::socket(io_service)),
    hostname_(hostname),
    nodename_(nodename),
    port_(port),
    peer_node_(peer_node),
    peer_port_(peer_port),
    io_service_(io_service)
{
    connect();
}

comms_client::~comms_client()
{
    close();
    if (message_loop_.joinable())
        message_loop_.join();
}

void comms_client::close()
{
    // queue a call to close the socket when the asio is idle
    io_service_.post([this]() { socket_.close(); });
}

void comms_client::connect()
{
    std::clog << "\nConnecting to " << peer_node_ << ":" << peer_port_ << " ...";
    auto endpoint_iterator = tcp::resolver(io_service_).resolve({ peer_node_, boost::lexical_cast<std::string>(peer_port_).c_str() });

    boost::asio::async_connect(
        socket_,
        endpoint_iterator,
        [this](boost::system::error_code ec, tcp::resolver::iterator)
        {
            if (ec)
            {
                error_code_ = ec;
                std::cerr << '\n' << error_code_.message();
            }
            else
            {
                std::clog << "\nConnected.\n";
                join_cluster();
                run_message_loop();
                read();
            }
        });
}

void comms_client::join_cluster()
{
    write(message::join_cluster(hostname_, port_, nodename_));
}

void comms_client::message_loop()
{
    std::string line;
    while (!error_code_  &&  getline(std::cin, line))
    {
        if (!error_code_)
            write(message::generic_text(std::move(line)));
    }
}

void comms_client::read()
{
    perform_read(
        [this](){
            std::cout << "==> ";
            std::cout.write(read_msg_.body(), read_msg_.body_length());
            std::cout << "\n";
            read();
        },
        [this](boost::system::error_code ec) {
            error_code_ = ec;
            socket_.close();
        });
}

void comms_client::run_message_loop()
{
    message_loop_ = std::thread(std::bind(&comms_client::message_loop, this));
}

void comms_client::write()
{
    perform_write(
        [this]() { write(); },
        [this](boost::system::error_code ec) {
            error_code_ = ec;
            socket_.close();
        });
}

void comms_client::write(message::generic_text msg)
{
    io_service_.post(
        [this, msg]() {
            bool write_in_progress = !write_msgs_.empty();
            write_msgs_.push_back(msg);
            if (!write_in_progress)
                write();
        });
}

}   // namespace clustery
