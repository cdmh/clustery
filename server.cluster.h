namespace clustery {

class cluster
{
  public:
    explicit cluster(int const recent_msg_count=100);
    void deliver(message const &msg);
    void join(cluster_member_ptr member);
    void leave(cluster_member_ptr member);

  private:
    int const                         recent_msg_count_;
    std::set<cluster_member_ptr>      members_;
    message_queue_t                   recent_msgs_;
    std::size_t                       message_count_;
    unsigned long                     cluster_number_;
    static std::atomic<unsigned long> cluster_count_;
};

}   // namespace clustery
