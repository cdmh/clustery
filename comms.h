namespace clustery {

using boost::asio::ip::tcp;

class comms
{
  protected:
    comms(tcp::socket &&socket)
      : socket_(std::forward<tcp::socket>(socket))
    { }

    void perform_read(std::function<void ()> on_success, std::function<void ()> on_error)
    {
        boost::asio::async_read(
            socket_,
            read_msg_.header_buffer(),
            [this, on_success, on_error](boost::system::error_code ec, std::size_t /*length*/)
            {
                if (!ec)
                    read_body(on_success, on_error);
                else
                    on_error();
            });
    }

    void perform_write(std::function<void ()> on_success, std::function<void ()> on_error)
    {
        boost::asio::async_write(
            socket_,
            write_msgs_.front().header_buffer(),
            [this, on_success, on_error](boost::system::error_code ec, std::size_t /*length*/)
            {
                if (ec)
                    on_error();
                else
                    write_body(on_success, on_error);
            });
    }

  private:
    void read_body(
        std::function<void ()> on_success,
        std::function<void ()> on_error)
    {
        boost::asio::async_read(
            socket_,
            read_msg_.body_buffer(),
            [this, on_success, on_error](boost::system::error_code ec, std::size_t /*length*/)
            {
                if (ec)
                    on_error();
                else
                    on_success();
            });
    }

    void write_body(std::function<void ()> on_success, std::function<void ()> on_error)
    {
        boost::asio::async_write(
            socket_,
            write_msgs_.front().body_buffer(),
            [this, on_success, on_error](boost::system::error_code ec, std::size_t /*length*/)
            {
                if (ec)
                    on_error();
                else
                {
                    write_msgs_.pop_front();
                    if (!write_msgs_.empty())
                        on_success();
                }
            });
    }

  protected:
    tcp::socket     socket_;
    message         read_msg_;
    message_queue_t write_msgs_;
};

}   // namespace clustery
