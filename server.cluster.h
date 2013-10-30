namespace clustery {

class cluster
{
  public:
    explicit cluster(std::size_t const recent_msg_count=100);
    void deliver(message const &msg, session_ptr from);
    void join(session_ptr member);
    void leave(session_ptr member);

  private:
    std::size_t const                 recent_msg_count_;
    std::set<session_ptr>             members_;
    message_queue_t                   recent_msgs_;
    std::size_t                       message_count_;
    unsigned long                     cluster_number_;
    static std::atomic<unsigned long> cluster_count_;
};

}   // namespace clustery
