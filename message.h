class message
{
  private:
    enum { max_body_length = 1024 };

  public:
    message(std::size_t length=0)
      : body_length_(length)      
    {
        header_.resize(4);

        if (body_length_ > max_body_length)
            body_length_ = max_body_length;
    }

    message(std::string const &msg) : message(msg.length())
    {
        std::memcpy(body_, msg.c_str(), body_length_);
        encode_header();
    }

    boost::asio::mutable_buffers_1 body_buffer()
    {
        decode_header();
        return boost::asio::buffer(body_, body_length_);
    }

    boost::asio::mutable_buffers_1 header_buffer()
    {
        return boost::asio::buffer(header_);
    }

    char const *body() const
    {
        return body_;
    }

    std::size_t const body_length() const
    {
        return body_length_;
    }

  private:
    bool const decode_header()
    {
        body_length_ = 0;
        for (auto ch : header_)
        {
            // we know there's only spaces at the beginning, so
            // don't need to handle more that this
            if (ch != ' ')
                body_length_ = (body_length_ * 10) + (ch-'0');
        }

        if (body_length_ > max_body_length)
        {
            body_length_ = 0;
            return false;
        }

        return true;
    }

    void encode_header()
    {
        auto it = header_.rbegin();
        for (std::size_t length=body_length_; length!=0; length/=10, ++it)
            *it = (length%10) + '0';
        for (; it!=header_.rend(); ++it)
            *it = ' ';
    }

  private:
    std::vector<char> header_;
    char              body_[max_body_length];
    std::size_t       body_length_;
};

typedef std::deque<message> message_queue_t;



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
