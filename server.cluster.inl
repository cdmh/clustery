namespace clustery {

cluster::cluster(boost::asio::io_service &io_service)
  : io_service_(io_service),
    message_count_(0),
    cluster_number_(++cluster_count_)
{
}

void cluster::process_inbound_message(message::generic_text msg)
{
    // deliver the message to all other servers in the cluster
    for (auto member : members_)
    {
        //if (member != from)
            member->deliver(msg);
    }

    std::clog << "\nCluster " << cluster_number_ << ", received message " << ++message_count_;
    switch (msg.id())
    {
        case message::join_cluster::message_type_id:
        {
            std::cout << "\n==> "; std::cout.write(msg.body(), msg.body_length());

            message::join_cluster join;
            if (!join.decode(msg.body(), msg.body_length()))
                throw std::runtime_error("Message decode failed.");
            std::cout << "\n--> " << join;
            break;
        }
    }
}

// this function is called while reading data from the network socket,
// so we don't perform processing here, but instead post a call to
// process the message later (moving the message for effiency)
void cluster::deliver(message::generic_text &&msg, session_ptr from)
{
    io_service_.post(
        std::bind(
            &cluster::process_inbound_message,
            this,
            std::forward<message::generic_text>(msg)));
}

void cluster::join(session_ptr member)
{
    members_.insert(member);
    std::clog << "\nServer joined. Now " << members_.size();
}

void cluster::leave(session_ptr member)
{
    members_.erase(member);
    std::clog << "\nServer left. Now " << members_.size();
}

std::atomic<unsigned long> cluster::cluster_count_(0);

}   // namespace clustery
