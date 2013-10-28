#include <boost/asio.hpp>

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

    std::size_t body_length() const
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
