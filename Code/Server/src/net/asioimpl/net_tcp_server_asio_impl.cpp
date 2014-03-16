#include "net_tcp_server_asio_impl.h"
#include "net_tcp_asio_session_impl.h"

namespace net
{

net_tcp_server_asio_impl::net_tcp_server_asio_impl(uint32 min_sendbuff, uint32 max_sendbuff, uint32 min_receivebuff, uint32 max_receivebuff)
	: timer_(boost::ref(*net_tcp_asio_core::instance().get_io_service())),
	min_sendbuff_(min_sendbuff),
	max_sendbuff_(max_sendbuff),
	min_receivebuff_(min_receivebuff),
	max_receivebuff_(max_receivebuff)
{
}

net_tcp_server_asio_impl::~net_tcp_server_asio_impl()
{
}

bool net_tcp_server_asio_impl::listen(const std::string& ip, uint16 port)
{
	if(ip.empty())
		return false;

	io_service_ptr io_service = get_io_service();
	if(!io_service)
		return false;

	// 构建监听器
	acceptor_ = boost::make_shared<boost::asio::ip::tcp::acceptor>(boost::ref(*io_service));
	if(!acceptor_)
		return false;

	// 初始化监听器
	boost::asio::ip::address address;
	address = boost::asio::ip::address::from_string(ip);
	boost::asio::ip::tcp::endpoint ep(address, port);
  
	acceptor_->open(boost::asio::ip::tcp::v4());
	acceptor_->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
	acceptor_->bind(ep);
	acceptor_->listen();
  
	// 发起异步接收
	// 生成一个新的等待连的会话, 这个新的会话并不会放入到已连接的会话列表中
	
	session_asio_ptr session = boost::make_shared<net_tcp_session_asio_impl>(boost::ref(*get_io_service()), this);
	if(!session)
		return false;

	acceptor_->async_accept(session->get_socket(), boost::bind(&net_tcp_server_asio_impl::handle_accept, this, 
						acceptor_, session, boost::asio::placeholders::error));
	
	return true;
}

void net_tcp_server_asio_impl::handle_accept(acceptor_ptr acceptor, session_ptr session, const boost::system::error_code& e)
{
	if(e)
	{
		// 发生错误了
		printf("----->  %s  <-----\n", e.message().c_str());
	}
	else
	{
		session->start(min_sendbuff_, max_sendbuff_, min_receivebuff_, max_receivebuff_);
		add_session(session);
	}

	if(!net_tcp_asio_core::instance().is_running())
		return;

	if(!acceptor_)
		return;

	if(!acceptor_->is_open())
		return;

	// 发起新的接收
	session_asio_ptr newsession = boost::make_shared<net_tcp_session_asio_impl>(boost::ref(*get_io_service()), this);
	if(!newsession)
		return;

	acceptor->async_accept(newsession->get_socket(), boost::bind(&net_tcp_server_asio_impl::handle_accept, this,  acceptor, 
		newsession, boost::asio::placeholders::error));	
}

void net_tcp_server_asio_impl::handle_timer(const boost::system::error_code& e)
{
	boost::recursive_mutex::scoped_lock autolock(session_container_mutex_);

	std::vector<session_ptr> delcontainer;

	for(BOOST_AUTO(it, session_container_.begin()); it != session_container_.end(); it++)
	{
		session_ptr session = *it;

		session->tick(1000);

		if(!session->is_valid())
		{
			delcontainer.push_back(session);
		}
	}

	for(BOOST_AUTO(it, delcontainer.begin()); it != delcontainer.end(); ++it)
	{
		remove_session(*it);
	}

	if(!session_container_.empty())
	{
		timer_.expires_from_now(boost::posix_time::seconds(1));
		timer_.async_wait(boost::bind(&net_tcp_server_asio_impl::handle_timer, shared_from_this(), boost::asio::placeholders::error));
	}
}

bool net_tcp_server_asio_impl::remove_session(session_ptr session)
{
	{
		boost::recursive_mutex::scoped_lock autolock(session_container_mutex_);

		BOOST_AUTO(it, std::find(session_container_.begin(), session_container_.end(), session));
		if(it == session_container_.end())
			return false;

		session_container_.erase(it);

		if(session)
		{
			session->close();
		}
	}

	{
		boost::recursive_mutex::scoped_lock autolock(newsession_container_mutex_);

		BOOST_AUTO(fit, std::find(newsession_container_.begin(), newsession_container_.end(), session));
		if(fit != newsession_container_.end())
		{
			newsession_container_.erase(fit);
		}

		if(session_container_.empty())
		{
			boost::system::error_code ec;
			timer_.cancel(ec);
		}
	}

	return true;
}

bool net_tcp_server_asio_impl::add_session(session_ptr session)
{
	{
		boost::recursive_mutex::scoped_lock autolock(session_container_mutex_);

		BOOST_AUTO(it, std::find(session_container_.begin(), session_container_.end(), session));
		if(it != session_container_.end())
			return false;

		session_container_.push_back(session);
	}

	{
		boost::recursive_mutex::scoped_lock autolock(newsession_container_mutex_);

		newsession_container_.push_back(session);
	
		if(session_container_.size() == 1)
		{
			timer_.expires_from_now(boost::posix_time::seconds(1));
			timer_.async_wait(boost::bind(&net_tcp_server_asio_impl::handle_timer, shared_from_this(), boost::asio::placeholders::error));
		}
	}
	return true;
}

session_ptr net_tcp_server_asio_impl::get_newsession()
{
	boost::recursive_mutex::scoped_lock autolock(newsession_container_mutex_);

	if(newsession_container_.empty())
	{
		return session_ptr();
	}

	session_ptr session = newsession_container_.front();
	newsession_container_.pop_front();
	return session;
}

io_service_ptr net_tcp_server_asio_impl::get_io_service()
{
	return net_tcp_asio_core::instance().get_io_service();
}

};