#include "net_tcp_client_asio_impl.h"

namespace net
{

net_tcp_client_asio_impl::net_tcp_client_asio_impl(uint32 min_sendbuff, uint32 max_sendbuff, uint32 min_receivebuff, uint32 max_receivebuff)
	: timer_(boost::ref(*net_tcp_asio_core::instance().get_io_service())),
	min_sendbuff_(min_sendbuff),
	max_sendbuff_(max_sendbuff),
	min_receivebuff_(min_receivebuff),
	max_receivebuff_(max_receivebuff)
{
}

net_tcp_client_asio_impl::~net_tcp_client_asio_impl()
{
}

bool net_tcp_client_asio_impl::async_connect(const std::string& ip, uint16 port)
{
	if(ip.empty())
		return false;

	if(!net_tcp_asio_core::instance().is_running())
		return false;

	boost::asio::ip::tcp::resolver resolver(*net_tcp_asio_core::instance().get_io_service());
	boost::asio::ip::tcp::resolver::query query(ip, boost::lexical_cast<std::string>(port));

	boost::system::error_code ec;
	boost::asio::ip::tcp::resolver::iterator it, end;

	it = resolver.resolve(query, ec);
	if(it == end || ec)
		return false;

	session_asio_ptr session = boost::make_shared<net_tcp_session_asio_impl>(boost::ref(*get_io_service()), this);

    session->get_socket().async_connect(*it,
        boost::bind(&net_tcp_client_asio_impl::handle_connect, shared_from_this(), session, ip, port, 
          boost::asio::placeholders::error));

	return true;
}

session_ptr net_tcp_client_asio_impl::connect(const std::string& ip, uint16 port)
{
	if(ip.empty())
		return session_ptr();

	if(!net_tcp_asio_core::instance().is_running())
		return session_ptr();

	boost::asio::ip::tcp::resolver resolver(*net_tcp_asio_core::instance().get_io_service());
	boost::asio::ip::tcp::resolver::query query(ip, boost::lexical_cast<std::string>(port));

	boost::system::error_code ec;
	boost::asio::ip::tcp::resolver::iterator it, end;

	it = resolver.resolve(query, ec);
	if(it == end || ec)
		return session_ptr();

	session_asio_ptr session = boost::make_shared<net_tcp_session_asio_impl>(boost::ref(*get_io_service()), this);
	if(!session)
		return session_asio_ptr();

	boost::system::error_code e;
	session->get_socket().connect(*it, e);
	if(e)
	{
		printf("----->  %s  <-----\n", e.message().c_str());
		return session_asio_ptr();
	}

	session->start(min_sendbuff_, max_sendbuff_, min_receivebuff_, max_receivebuff_);
	session->send_tgw(ip, port);
	add_session(session);
	return session;
}

void net_tcp_client_asio_impl::handle_connect(session_ptr session, const std::string& ip, uint16 port, const boost::system::error_code& e)
{
	if(e)
	{
		// ·¢Éú´íÎóÁË
		printf("----->  %s  <-----\n", e.message().c_str());
	}
	else
	{
		session->start(min_sendbuff_, max_sendbuff_, min_receivebuff_, max_receivebuff_);
		session->send_tgw(ip, port);

		add_session(session);

		boost::recursive_mutex::scoped_lock autolock(newsession_container_mutex_);

		newsession_container_.push_back(session);
	}

}

void net_tcp_client_asio_impl::handle_timer(const boost::system::error_code& e)
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
		timer_.async_wait(boost::bind(&net_tcp_client_asio_impl::handle_timer, shared_from_this(), boost::asio::placeholders::error));
	}
}

bool net_tcp_client_asio_impl::remove_session(session_ptr session)
{
	{
		boost::recursive_mutex::scoped_lock autolock(session_container_mutex_);

		BOOST_AUTO(it, std::find(session_container_.begin(), session_container_.end(), session));
		if(it == session_container_.end())
			return false;

		session_container_.erase(it);

		if(session->is_connect())
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

bool net_tcp_client_asio_impl::add_session(session_ptr session)
{
	boost::recursive_mutex::scoped_lock autolock(session_container_mutex_);

	BOOST_AUTO(it, std::find(session_container_.begin(), session_container_.end(), session));
	if(it != session_container_.end())
		return false;

	session_container_.push_back(session);

	if(session_container_.size() == 1)
	{
		timer_.expires_from_now(boost::posix_time::seconds(1));
		timer_.async_wait(boost::bind(&net_tcp_client_asio_impl::handle_timer, shared_from_this(), boost::asio::placeholders::error));
	}
	return true;
}

session_ptr net_tcp_client_asio_impl::get_newsession()
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

io_service_ptr net_tcp_client_asio_impl::get_io_service()
{
	return net_tcp_asio_core::instance().get_io_service();
}

};