#include "net_tcp_asio_impl.h"

namespace net
{

net_tcp_asio_impl::net_tcp_asio_impl() : next_io_service_(0)
{
}

net_tcp_asio_impl::~net_tcp_asio_impl()
{

}

bool net_tcp_asio_impl::init(uint16 threadcnt)
{
	if(threadcnt == 0)
		return false;

	for(int i = 0; i < threadcnt; i++)
	{
		io_service_ptr io_service = boost::make_shared<boost::asio::io_service>();
		io_service_work_ptr io_service_work = boost::make_shared<boost::asio::io_service::work>(boost::ref(*io_service));
    
		io_service_container_.push_back(io_service);
		io_service_work_container_.push_back(io_service_work);

		boost::system::error_code ec;
		thread_ptr thread = boost::make_shared<boost::thread>(boost::bind(&boost::asio::io_service::run, io_service.get(), ec));

		thread_container_.push_back(thread);
	}

	running_ = true;
	return true;
}

io_service_ptr net_tcp_asio_impl::get_io_service()
{
	boost::recursive_mutex::scoped_lock autolock(service_mutex_);

	if(io_service_container_.empty())
		return io_service_ptr();

	if(next_io_service_ >= io_service_container_.size())
		next_io_service_ = 0;

	return io_service_container_[next_io_service_++];
}

void net_tcp_asio_impl::destory()
{
	boost::recursive_mutex::scoped_lock autolock(service_mutex_);

	for(std::size_t i = 0; i < io_service_container_.size(); i++)
	{
		io_service_container_[i]->stop();
	}

	for(std::size_t i = 0; i < thread_container_.size(); i++)
	{
		thread_container_[i]->join();
	}
}

bool net_tcp_asio_impl::is_running()
{
	boost::recursive_mutex::scoped_lock autolock(service_mutex_);
	return running_;
}

};