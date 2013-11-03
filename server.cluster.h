#include <mutex>

namespace clustery {

class cluster
{
  public:
    explicit cluster(boost::asio::io_service &io_service);
    void deliver(message::generic_text &&msg, session_ptr from);
    void join(session_ptr member);
    void leave(session_ptr member);
    void process_inbound_message(message::generic_text msg);

  private:
    boost::asio::io_service &io_service_;
    std::atomic<std::size_t> message_count_;
    unsigned long            cluster_number_;
    std::set<session_ptr>    members_;

    static std::atomic<unsigned long> cluster_count_;

    cluster(cluster &&)                 = delete;
    cluster(cluster const &)            = delete;
    cluster &operator=(cluster &&)      = delete;
    cluster &operator=(cluster const &) = delete;
};

}   // namespace clustery
