#include "comms.h"

namespace clustery {

class comms_client : public comms
{
  public:
    comms_client(
        char const              *hostname,
        char const              *nodename,
        unsigned short           port,
        boost::asio::io_service &io_service,
        char const              *peer_node,
        unsigned short           peer_port);
    ~comms_client();
    void close();

  private:
    void connect();
    void message_loop();
    void join_cluster();
    void read();
    void run_message_loop();
    void write();
    void write(message::generic_text &&msg);

  private:
    char const *         const hostname_;
    char const *         const nodename_;
    unsigned short       const port_;
    char const *         const peer_node_;
    unsigned short       const peer_port_;
    std::thread                message_loop_;
    boost::asio::io_service   &io_service_;
    boost::system::error_code  error_code_;
};

}   // namespace clustery
