namespace clustery {

void session::deliver(message::generic_text const &msg)
{
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(msg);
    if (!write_in_progress)
        write();
}

void session::read()
{
    perform_read(
        [this](){
            cluster_.deliver(read_msg_, shared_from_this());
            read();
        },
        std::bind(&cluster::leave, &cluster_, shared_from_this()));
}

void session::start()
{
    cluster_.join(shared_from_this());
    read();
}

void session::write()
{
    perform_write(
        std::bind(&session::write, this),
        std::bind(&cluster::leave, &cluster_, shared_from_this()));
}

}   // namespace clustery
