#ifndef __net_tcp_asio_impl_h__
#define __net_tcp_asio_impl_h__

//#include <boost/pool/pool.hpp>

#include "def.h"
#include "net.h"

#include <boost/asio.hpp>

namespace net
{

typedef boost::shared_ptr<boost::asio::io_service> io_service_ptr;

class net_tcp_asio_impl
{
public:
	static net_tcp_asio_impl& instance()
	{
		static net_tcp_asio_impl instanceobj;
		return instanceobj;
	}

public:
	typedef boost::shared_ptr<boost::asio::io_service::work> io_service_work_ptr;
	typedef boost::shared_ptr<boost::thread> thread_ptr;

	net_tcp_asio_impl();
	~net_tcp_asio_impl();

public:
	bool init(uint16 threadcnt);
	void destory();
	io_service_ptr get_io_service();

	bool is_running();

private:
	boost::recursive_mutex				service_mutex_;

	std::vector<io_service_ptr>			io_service_container_;
	std::vector<io_service_work_ptr>	io_service_work_container_;
	std::size_t							next_io_service_;
	bool								running_;

	//线程集合, 每个线程为一个io_service服务
	std::vector<thread_ptr> thread_container_;
};

typedef net_tcp_asio_impl net_tcp_asio_core;

};

#endif