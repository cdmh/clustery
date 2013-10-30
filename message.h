namespace clustery {

namespace message {

// TODO detect and support endianness. Currently, all systems in the cluster must be the same endianness
class generic_text
{
  public:
    static std::uint16_t const message_type_id = 1;

    generic_text(std::uint32_t length=0)
      : head({message_type_id, length})
    {
        if (head.body_length_ > 0)
            body_.resize(head.body_length_);
    }

    generic_text(std::string &&msg, std::uint16_t const id=message_type_id)
        : generic_text((std::uint32_t)msg.length())
    {
        head.msg_id_ = id;
        if (head.body_length_ > 0)
            std::memcpy(&body_[0], msg.c_str(), head.body_length_);
    }

    boost::asio::mutable_buffers_1 body_buffer()
    {
        decode_header();
        return boost::asio::buffer(body_, head.body_length_);
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

    std::uint32_t const body_length() const
    {
        return head.body_length_;
    }

  private:
    bool const decode_header()
    {
        if (head.body_length_ > body_.size())
            body_.resize(head.body_length_);
        return true;
    }

  private:
    struct header {
        // this struct uses fixed size types for
        // interoperability between 32/64 systems
        std::uint16_t msg_id_;
        std::uint32_t body_length_;
    };
    union {
        header head;
        char   header_[sizeof(header)];
    };
    std::vector<char> body_;
};

class join_cluster
{
  public:
    static std::uint16_t const message_type_id = 2;

    join_cluster(char const *hostname, int port, char const *node)
      : hostname_(hostname), port_(port), node_(node)
    {
    }

    operator generic_text() const
    {
        std::ostringstream msgtxt;
        msgtxt << "join-cluster: " << hostname_ << ":" << port_ << " (" << node_ << ")";
        return generic_text(msgtxt.str(), message_type_id);
    }

  private:
    int         port_;
    char const *hostname_;
    char const *node_;
};

typedef std::deque<generic_text> queue_t;

}   // namespace message


}   //  namespace clustery
