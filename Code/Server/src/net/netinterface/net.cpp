#include "net.h"

#include "net_tcp_asio_impl.h"
#include "net_tcp_server_asio_impl.h"
#include "net_tcp_client_asio_impl.h"

namespace net
{

tcp_server_ptr create_tcp_server(uint32 min_sendbuff, uint32 max_sendbuff, uint32 min_receivebuff, uint32 max_receivebuff)
{
	return boost::make_shared<net_tcp_server_asio_impl>(min_sendbuff, max_sendbuff, min_receivebuff, max_receivebuff);
}

tcp_client_ptr creat_tcp_client(uint32 min_sendbuff, uint32 max_sendbuff, uint32 min_receivebuff, uint32 max_receivebuff)
{
	return boost::make_shared<net_tcp_client_asio_impl>(min_sendbuff, max_sendbuff, min_receivebuff, max_receivebuff);
}

bool net_init(uint16 threadcnt)
{
	return net_tcp_asio_core::instance().init(threadcnt);
}

void net_clear()
{
	net_tcp_asio_core::instance().destory();
}

};