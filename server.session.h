namespace clustery {

class cluster;
class session
  : protected comms,
    public std::enable_shared_from_this<session>
{
  public:
    session(tcp::socket &&socket, cluster &c)
      : comms(std::forward<tcp::socket>(socket)),
        cluster_(c)
    { }

    void deliver(message::generic_text const &msg);
    void start();

  private:
    void read();
    void write();


  private:
    cluster &cluster_;
};
typedef std::shared_ptr<session> session_ptr;

}   // namespace clustery
