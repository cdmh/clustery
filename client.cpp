#include "stdafx.h"
#include "message.h"
#include "comms.h"

namespace clustery {

class comms_client : public comms
{
  public:
    comms_client(boost::asio::io_service &io_service, tcp::resolver::iterator endpoint_iterator)
      : comms(tcp::socket(io_service)),
        connected_(false),
        io_service_(io_service)
    {
        connect(endpoint_iterator);
    }

    boost::system::error_code const error_code()
    {
        return error_code_;
    }

    std::string error() const
    {
        return error_code_.message();
    }

    bool const connected() const
    {
        return connected_;
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
                if (ec)
                    error_code_ = ec;
                else
                {
                    connected_ = true;
                    read();
                }
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
            [this](boost::system::error_code ec) {
                socket_.close();
                error_code_ = ec;
            });
    }

    void write()
    {
        perform_write(
            [this]() { write(); },
            [this](boost::system::error_code ec) {
                socket_.close();
                error_code_ = ec;
            });
    }

  private:
    bool                       connected_;
    boost::asio::io_service   &io_service_;
    boost::system::error_code  error_code_;
};

void client(boost::asio::io_service &io_service, char const *hostname, int port, char const *node)
{
    std::cout << "\nClustery client\n===============\n\nNode: " << node << "\n";

    auto endpoint_iterator = tcp::resolver(io_service).resolve({ hostname, boost::lexical_cast<std::string>(port).c_str() });
    comms_client c(io_service, endpoint_iterator);

    std::cout << "\nConnecting...";
    while (!c.error_code()  &&  !c.connected())
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

    if (c.error_code())
    {
        std::cout << '\n' << c.error();
        return;
    }

    std::cout << "\nConnected.\n";
    message msg(std::string("join-cluster: ") + node);
    c.write(msg);

    std::string line;
    while (!c.error_code()  &&  getline(std::cin, line))
    {
        if (!c.error_code())
        {
            message msg(line);
            c.write(msg);
        }
    }

    c.close();
}

}   // namespace clustery
