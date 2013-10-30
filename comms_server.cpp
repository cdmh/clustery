#include "stdafx.h"
#include "comms.h"

#include "server.session.h"
#include "server.cluster.h"
#include "server.session.inl"
#include "server.cluster.inl"

namespace clustery {

class comms_server
{
  public:
    comms_server(boost::asio::io_service &io_service, const tcp::endpoint &endpoint)
    : acceptor_(io_service, endpoint),
      socket_(io_service)
    {
        accept();
    }

  private:
    void accept()
    {
        acceptor_.async_accept(socket_,
            [this](boost::system::error_code ec)
            {
                if (ec)
                    std::cerr << ec.message();
                else
                    std::make_shared<session>(std::move(socket_), cluster_)->start();

                accept();
            });
    }

  private:
    tcp::acceptor acceptor_;
    tcp::socket   socket_;
    cluster       cluster_;
};


void server(boost::asio::io_service &io_service, int *ports, int num_ports)
{
    try
    {
        static std::list<comms_server> servers;
        for (int i=0; i < num_ports; ++i)
        {
            std::cout << "\nListening on port " << ports[i];
            tcp::endpoint endpoint(tcp::v4(), ports[i]);
            servers.emplace_back(io_service, endpoint);
        }
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}

}   // namespace clustery
