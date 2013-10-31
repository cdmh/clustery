namespace clustery {

cluster::cluster(std::size_t const recent_msg_count)
  : recent_msg_count_(recent_msg_count),
    message_count_(0),
    cluster_number_(++cluster_count_)
{
}

void cluster::deliver(message::generic_text const &msg, session_ptr from)
{
    std::clog << "\nCluster " << cluster_number_ << ", received message " << ++message_count_;

    switch (msg.id())
    {
        case message::join_cluster::message_type_id:
        {
            message::join_cluster join;
            if (!join.decode(msg.body(), msg.body_length()))
                throw std::runtime_error("Message decode failed.");
            std::cout << "\n==> "; std::cout.write(msg.body(), msg.body_length());
            std::cout << "\n--> " << join;
            break;
        }
    }

    // store the message for sending to new servers as they join the cluster
    recent_msgs_.push_back(msg);
    while (recent_msgs_.size() > recent_msg_count_)
        recent_msgs_.pop_front();

    // deliver the message to all other servers in the cluster
    for (auto member : members_)
    {
        if (member != from)
            member->deliver(msg);
    }
}

void cluster::join(session_ptr member)
{
    members_.insert(member);
    std::clog << "\nServer joined. Now " << members_.size();
    for (auto msg : recent_msgs_)
        member->deliver(msg);
}

void cluster::leave(session_ptr member)
{
    members_.erase(member);
    std::clog << "\nServer left. Now " << members_.size();
}

std::atomic<unsigned long> cluster::cluster_count_(0);

}   // namespace clustery
