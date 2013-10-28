#include "stdafx.h"
#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <utility>
#include <boost/asio.hpp>
#include "message.h"

using boost::asio::ip::tcp;

//----------------------------------------------------------------------

typedef std::deque<chat_message> chat_message_queue;

//----------------------------------------------------------------------

class chat_participant
{
public:
  virtual ~chat_participant() {}
  virtual void deliver(const chat_message& msg) = 0;
};

typedef std::shared_ptr<chat_participant> chat_participant_ptr;

//----------------------------------------------------------------------

class chat_room
{
public:
  void join(chat_participant_ptr participant)
  {
    participants_.insert(participant);
    for (auto msg: recent_msgs_)
      participant->deliver(msg);
  }

  void leave(chat_participant_ptr participant)
  {
    participants_.erase(participant);
  }

  void deliver(const chat_message& msg)
  {
    recent_msgs_.push_back(msg);
    while (recent_msgs_.size() > max_recent_msgs)
      recent_msgs_.pop_front();

    for (auto participant: participants_)
      participant->deliver(msg);
  }

private:
  std::set<chat_participant_ptr> participants_;
  enum { max_recent_msgs = 100 };
  chat_message_queue recent_msgs_;
};

//----------------------------------------------------------------------

class chat_session
  : public chat_participant,
    public std::enable_shared_from_this<chat_session>
{
public:
  chat_session(tcp::socket socket, chat_room& room)
    : socket_(std::move(socket)),
      room_(room)
  {
  }

  void start()
  {
    room_.join(shared_from_this());
    do_read_header();
  }

  void deliver(const chat_message& msg)
  {
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(msg);
    if (!write_in_progress)
    {
      do_write();
    }
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
                    room_.leave(shared_from_this());
            });
    }

    void do_read_body()
    {
        auto self(shared_from_this());

        boost::asio::async_read(socket_, read_msg_.body_buffer(),
            [this, self](boost::system::error_code ec, std::size_t /*length*/)
            {
                if (!ec)
                    room_.deliver(read_msg_);
                else
                    room_.leave(shared_from_this());
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
                    room_.leave(shared_from_this());
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
                    room_.leave(shared_from_this());
            });
    }

  tcp::socket socket_;
  chat_room& room_;
  chat_message read_msg_;
  chat_message_queue write_msgs_;
};

//----------------------------------------------------------------------

class chat_server
{
public:
  chat_server(boost::asio::io_service& io_service,
      const tcp::endpoint& endpoint)
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
          {
            std::make_shared<chat_session>(std::move(socket_), room_)->start();
          }

          do_accept();
        });
  }

  tcp::acceptor acceptor_;
  tcp::socket socket_;
  chat_room room_;
};

//----------------------------------------------------------------------

void server(int *ports, int num_ports)
{
    std::cout << "\nClustery server\n===============\nRunning ...";
    try
    {
        boost::asio::io_service io_service;

        std::list<chat_server> servers;
        for (int i=0; i < num_ports; ++i)
        {
            tcp::endpoint endpoint(tcp::v4(), ports[i]);
            servers.emplace_back(io_service, endpoint);
        }

        io_service.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}
