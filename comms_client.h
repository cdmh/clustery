#include "comms.h"

namespace clustery {

class comms_client : public comms
{
  public:
    comms_client(char const *node, boost::asio::io_service &io_service, char const *hostname, int port);
    ~comms_client();
    void close();

  private:
    void connect(tcp::resolver::iterator endpoint_iterator);
    void message_loop();
    void join_cluster();
    void read();
    void run_message_loop();
    void write();
    void write(message::generic_text &&msg);

  private:
    char const * const         node_;
    char const * const         hostname_;
    std::thread                message_loop_;
    boost::asio::io_service   &io_service_;
    boost::system::error_code  error_code_;
};

}   // namespace clustery
