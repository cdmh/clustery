#include "stdafx.h"
#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <utility>
#include <boost/asio.hpp>
#include <atomic>
#include "message.h"

using boost::asio::ip::tcp;

class cluster_participant
{
public:
  virtual ~cluster_participant() {}
  virtual void deliver(const message &msg) = 0;
};

typedef std::shared_ptr<cluster_participant> cluster_participant_ptr;



class cluster
{
  public:
    explicit cluster(int const recent_msg_count=100)
      : recent_msg_count_(recent_msg_count),
        message_count_(0),
        cluster_number_(++cluster_count_)
    {
    }

    void join(cluster_participant_ptr participant)
    {
        participants_.insert(participant);
        for (auto msg : recent_msgs_)
            participant->deliver(msg);
    }

    void leave(cluster_participant_ptr participant)
    {
        participants_.erase(participant);
    }

    void deliver(const message &msg)
    {
        std::cout << "\nRoom " << cluster_number_ << ", received message " << ++message_count_ << "\n--> ";
        std::cout.write(msg.body(), msg.body_length());

        // store the message for sending to new servers
        // as they join the cluster
        recent_msgs_.push_back(msg);
        while (recent_msgs_.size() > recent_msg_count_)
            recent_msgs_.pop_front();

        for (auto participant: participants_)
            participant->deliver(msg);
    }

  private:
    int const                         recent_msg_count_;
    std::set<cluster_participant_ptr> participants_;
    message_queue_t                   recent_msgs_;
    std::size_t                       message_count_;
    unsigned long                     cluster_number_;
    static std::atomic<unsigned long> cluster_count_;
};
std::atomic<unsigned long> cluster::cluster_count_(0);



class session
  : public cluster_participant,
    public std::enable_shared_from_this<session>
{
  public:
    session(tcp::socket socket, cluster &room)
      : socket_(std::move(socket)),
        cluster_(room)
    { }

    void start()
    {
        cluster_.join(shared_from_this());
        do_read_header();
    }

    void deliver(const message &msg)
    {
        bool write_in_progress = !write_msgs_.empty();
        write_msgs_.push_back(msg);
        if (!write_in_progress)
            do_write();
    }

  private:
    void do_read_header() // !!!duplicated function
    {
        auto self(shared_from_this());
        boost::asio::async_read(socket_, read_msg_.header_buffer(),
            [this, self](boost::system::error_code ec, std::size_t /*length*/)
            {
                if (!ec)
                    do_read_body();
                else
                    cluster_.leave(shared_from_this());
            });
    }

    void do_read_body()
    {
        auto self(shared_from_this());

        boost::asio::async_read(socket_, read_msg_.body_buffer(),
            [this, self](boost::system::error_code ec, std::size_t /*length*/)
            {
                if (!ec)
                {
                    cluster_.deliver(read_msg_);
                    do_read_header();
                }
                else
                {
                    std::cout << "Client left";
                    cluster_.leave(shared_from_this());
                }
            });
    }

    void do_write() //!!! duplicate function
    {
        auto self(shared_from_this());
        boost::asio::async_write(socket_, write_msgs_.front().header_buffer(),
            [this, self](boost::system::error_code ec, std::size_t /*length*/)
            {
                if (!ec)
                    do_body_write();
                else
                    cluster_.leave(shared_from_this());
            });
    }

    void do_body_write() //!!! duplicate function
    {
        auto self(shared_from_this());
        boost::asio::async_write(socket_, write_msgs_.front().body_buffer(),
            [this, self](boost::system::error_code ec, std::size_t /*length*/)
            {
                if (!ec)
                {
                    write_msgs_.pop_front();
                    if (!write_msgs_.empty())
                        do_write();
                }
                else
                    cluster_.leave(shared_from_this());
            });
    }

  private:
    tcp::socket     socket_;
    cluster        &cluster_;
    message         read_msg_;
    message_queue_t write_msgs_;
};



class comms_server
{
  public:
    comms_server(boost::asio::io_service &io_service, const tcp::endpoint &endpoint)
    : acceptor_(io_service, endpoint),
      socket_(io_service)
    {
        do_accept();
    }

  private:
    void do_accept()
    {
        acceptor_.async_accept(socket_,
            [this](boost::system::error_code ec)
            {
                if (!ec)
                    std::make_shared<session>(std::move(socket_), cluster_)->start();

                do_accept();
            });
    }

  private:
    tcp::acceptor acceptor_;
    tcp::socket   socket_;
    cluster       cluster_;
};




void server(int *ports, int num_ports)
{
    std::cout << "\nClustery server\n===============\nRunning ...";
    try
    {
        boost::asio::io_service io_service;

        std::list<comms_server> servers;
        for (int i=0; i < num_ports; ++i)
        {
            tcp::endpoint endpoint(tcp::v4(), ports[i]);
            servers.emplace_back(io_service, endpoint);
        }

        io_service.run();
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}
