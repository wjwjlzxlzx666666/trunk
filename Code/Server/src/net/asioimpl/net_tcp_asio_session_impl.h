#ifndef __net_tcp_asio_session_h__
#define __net_tcp_asio_session_h__

#include "net.h"
#include "net_def.h"

#include "net_tcp_asio_impl.h"

#include "../util/circular_buffer.h"

#define NET_DATA_BUFFER_LENGTH 4096

namespace net
{

class net_tcp_session_asio_impl : 
	public net_tcp_session_interface, 
	public boost::enable_shared_from_this<net_tcp_session_asio_impl>
{
public:
	net_tcp_session_asio_impl(boost::asio::io_service& io_service, net_tcp_session_mgr_interface* mgr);
	~net_tcp_session_asio_impl();

public:
	std::string get_remote_address() const;
	uint16 get_remote_port() const;
	std::string get_local_address() const;
	uint16 get_local_port() const;
	bool send_packet(uint8* buff, uint32 length);
	bool peek_packet(uint32& length);
	int32 receive_packet(uint8* buff, uint32 bufflength);
	bool is_connect() const;
	bool is_valid() const;
	virtual void start(uint32 min_sendbuff = DEFAULT_SEND_BUFFER_LENGTH, uint32 max_sendbuff = DEFAULT_SEND_BUFFER_LENGTH, uint32 min_receivebuff = DEFAULT_RECEIVE_BUFFER_LENGTH, uint32 max_receivebuff = DEFAULT_RECEIVE_BUFFER_LENGTH);
	void stop();										
	std::string get_last_error() const;
	void close();

public:
	void send_tgw(const std::string& ip, uint16 port);

public:
	boost::asio::ip::tcp::socket& get_socket(){return socket_;}

private:
	void handle_session_read_msg_data(bool readhead, const boost::system::error_code& e, std::size_t bytes_transferred);
	void handle_session_write_msg_data(const boost::system::error_code& e, std::size_t bytes_transferred);

	void tick(uint32 elapsed);
	void send_hearbeat();

	void close(const std::string& reason, bool force = false);

private:
	boost::asio::ip::tcp::socket	socket_;
	net_tcp_session_mgr_interface*	mgr_;

	boost::recursive_mutex			receive_queue_mutex_;
	boost::recursive_mutex			send_queue_mutex_;
	
	boost::recursive_mutex			close_mutex_;

	volatile bool					valid_;
	bool							closed_;
	bool							sending_;
	bool							start_;

	std::vector<uint8>				send_buffer_;
	std::vector<uint8>				receive_buffer_;

	boost::shared_ptr<circular_buffer>	send_msg_buffer_;
	boost::shared_ptr<circular_buffer>	receive_msg_buffer_;

	std::vector<uint8>				compress_buffer_;
	std::vector<uint8>				uncompress_buffer_;

	std::time_t						last_msg_time_;

	std::string						last_error_info_;

	uint16							remote_msg_index_;
	uint16							myself_msg_index_;
};

typedef boost::shared_ptr<net_tcp_session_asio_impl> session_asio_ptr;

};

#endif
