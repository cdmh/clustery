#include "stdafx.h"
#include "message.h"
#include "comms.h"

namespace clustery {

class cluster_member
{
  public:
    virtual ~cluster_member() {}
    virtual void deliver(message const &msg) = 0;
};

typedef std::shared_ptr<cluster_member> cluster_member_ptr;



class cluster
{
  public:
    explicit cluster(int const recent_msg_count=100)
      : recent_msg_count_(recent_msg_count),
        message_count_(0),
        cluster_number_(++cluster_count_)
    {
    }

    void join(cluster_member_ptr participant)
    {
        members_.insert(participant);
        std::clog << "\nParticipant joined. Now " << members_.size();
        for (auto msg : recent_msgs_)
            participant->deliver(msg);
    }

    void leave(cluster_member_ptr participant)
    {
        members_.erase(participant);
        std::clog << "\nParticipant left. Now " << members_.size();
    }

    void deliver(message const &msg)
    {
        std::clog << "\nRoom " << cluster_number_ << ", received message " << ++message_count_ << "\n--> ";
        std::clog.write(msg.body(), msg.body_length());

        // store the message for sending to new servers
        // as they join the cluster
        recent_msgs_.push_back(msg);
        while (recent_msgs_.size() > recent_msg_count_)
            recent_msgs_.pop_front();

        for (auto participant: members_)
            participant->deliver(msg);
    }

  private:
    int const                         recent_msg_count_;
    std::set<cluster_member_ptr>      members_;
    message_queue_t                   recent_msgs_;
    std::size_t                       message_count_;
    unsigned long                     cluster_number_;
    static std::atomic<unsigned long> cluster_count_;
};
std::atomic<unsigned long> cluster::cluster_count_(0);



class session
  : public cluster_member,
    public comms,
    public std::enable_shared_from_this<session>
{
  public:
    session(tcp::socket &&socket, cluster &room)
      : comms(std::forward<tcp::socket>(socket)),
        cluster_(room)
    { }

    void start()
    {
        cluster_.join(shared_from_this());
        read();
    }

    void deliver(message const &msg)
    {
        bool write_in_progress = !write_msgs_.empty();
        write_msgs_.push_back(msg);
        if (!write_in_progress)
            write();
    }

  private:
    void read()
    {
        perform_read(
            [this](){
                cluster_.deliver(read_msg_);
                read();
            },
            std::bind(&cluster::leave, &cluster_, shared_from_this()));
    }

    void write()
    {
        perform_write(
            std::bind(&session::write, this),
            std::bind(&cluster::leave, &cluster_, shared_from_this()));
    }


  private:
    cluster &cluster_;
};



class comms_server
{
  public:
    comms_server(boost::asio::io_service &io_service, const tcp::endpoint &endpoint)
    : acceptor_(io_service, endpoint),
      socket_(io_service)
    {
        accept();
    }

  private:
    void accept()
    {
        acceptor_.async_accept(socket_,
            [this](boost::system::error_code ec)
            {
                if (!ec)
                    std::make_shared<session>(std::move(socket_), cluster_)->start();

                accept();
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

}   // namespace clustery
