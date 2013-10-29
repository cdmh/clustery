namespace clustery {

class message
{
  public:
    message(std::size_t length=0)
      : body_length_(length)      
    {
        body_.resize(body_length_);
    }

    message(std::string const &msg) : message(msg.length())
    {
        if (body_length_ > 0)
            std::memcpy(&body_[0], msg.c_str(), body_length_);
        encode_header();
    }

    boost::asio::mutable_buffers_1 body_buffer()
    {
        decode_header();
        return boost::asio::buffer(body_, body_length_);
    }

    boost::asio::mutable_buffers_1 header_buffer()
    {
        static_assert(sizeof(header_[0]) == 1, "header should be bytes");
        return boost::asio::buffer(header_, sizeof(header_));
    }

    char const *body() const
    {
        return (body_.size() == 0)? "" : &body_[0];
    }

    std::size_t const body_length() const
    {
        return body_length_;
    }

  private:
    bool const decode_header()
    {
        body_length_ = 0;
        for (int loop=0; loop<sizeof(header_); ++loop)
            body_length_ = (body_length_ * 10) + (header_[loop] - '0');

        if (body_length_ > body_.size())
            body_.resize(body_length_);
        return true;
    }

    void encode_header()
    {
        char *p = header_ + sizeof(header_) - 1;
        for (std::size_t length=body_length_; length!=0; length/=10, --p)
            *p = (length % 10) + '0';
        for (; p >= header_; --p)
            *p = '0';
    }

  private:
    char              header_[4];
    std::vector<char> body_;
    std::size_t       body_length_;
};

typedef std::deque<message> message_queue_t;

}   //  namespace clustery
