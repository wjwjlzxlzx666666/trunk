#ifndef __net_tcp_server_impl_h__
#define __net_tcp_server_impl_h__

#include "net.h"

#include <boost/asio.hpp>

#include "net_tcp_asio_impl.h"
#include "net_tcp_asio_session_impl.h"

namespace net
{

class net_tcp_server_asio_impl : public net_tcp_server_interface,
	public boost::enable_shared_from_this<net_tcp_server_asio_impl>
{
public:
	typedef boost::shared_ptr<boost::asio::ip::tcp::acceptor> acceptor_ptr;
	
	net_tcp_server_asio_impl(uint32 min_sendbuff, uint32 max_sendbuff, uint32 min_receivebuff, uint32 max_receivebuff);
	~net_tcp_server_asio_impl();

public:
	bool listen(const std::string& ip, uint16 port);
	bool remove_session(session_ptr session);
	bool add_session(session_ptr session);
	session_ptr get_newsession();

	bool start(uint16 threadcnt);
	void stop();

	//内部函数声明
private:
	// Handle completion of an asynchronous accept operation.  
	void handle_accept(acceptor_ptr acceptor, session_ptr session, const boost::system::error_code& e); 
	void handle_timer(const boost::system::error_code& e);

	io_service_ptr get_io_service();

	//内部变量声明
private:
	acceptor_ptr	acceptor_;

	//会话容器,用来保存所有客户端的连接
	boost::recursive_mutex session_container_mutex_;
	std::list<session_ptr> session_container_;

	// 异步连接
	boost::recursive_mutex newsession_container_mutex_;
	std::list<session_ptr> newsession_container_;

	boost::asio::deadline_timer timer_;

	uint32 min_sendbuff_;
	uint32 max_sendbuff_;
	uint32 min_receivebuff_;
	uint32 max_receivebuff_;
};

typedef net_tcp_server_asio_impl server_asio;

};
#endif
