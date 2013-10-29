namespace clustery {

class cluster;
class session
  : protected comms,
    public std::enable_shared_from_this<session>
{
  public:
    session(tcp::socket &&socket, cluster &room)
      : comms(std::forward<tcp::socket>(socket)),
        cluster_(room)
    { }

    void deliver(message const &msg);
    void start();

  private:
    void read();
    void write();


  private:
    cluster &cluster_;
};
typedef std::shared_ptr<session> cluster_member_ptr;

}   // namespace clustery
