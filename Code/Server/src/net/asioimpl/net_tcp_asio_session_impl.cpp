#include "net_tcp_asio_session_impl.h"
#include "net_tcp_server_asio_impl.h"
#include <stdlib.h>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include "zlib.h"
namespace net
{

net_tcp_session_asio_impl::net_tcp_session_asio_impl(boost::asio::io_service& io_service, net_tcp_session_mgr_interface* mgr) : 
	socket_(io_service), mgr_(mgr), valid_(true), closed_(false), sending_(false), start_(false), remote_msg_index_(0), myself_msg_index_(0), 
	send_buffer_(NET_DATA_BUFFER_LENGTH + sizeof(datahead)),
	receive_buffer_(NET_DATA_BUFFER_LENGTH + sizeof(datahead)),
	compress_buffer_(NET_DATA_BUFFER_LENGTH),
	uncompress_buffer_(NET_DATA_BUFFER_LENGTH)
{

}

net_tcp_session_asio_impl::~net_tcp_session_asio_impl()
{
}

std::string net_tcp_session_asio_impl::get_remote_address() const
{
	if(!is_connect())
		return std::string();

	boost::system::error_code e;
	boost::asio::ip::tcp::endpoint endpoint = socket_.remote_endpoint(e);
	if(e)
	{
		return std::string("");
	}
		
	boost::asio::ip::address  address = endpoint.address();
	return address.to_string();
}

uint16 net_tcp_session_asio_impl::get_remote_port() const
{
	if(!is_connect())
		return (uint16)-1;
	
	boost::system::error_code e;
	boost::asio::ip::tcp::endpoint endpoint = socket_.remote_endpoint(e);
	if(e)
	{
		return (uint16)-1;
	}
	
	return endpoint.port();
}

std::string net_tcp_session_asio_impl::get_local_address() const
{
	if(!is_connect())
		return std::string();

	boost::system::error_code e;
	boost::asio::ip::tcp::endpoint endpoint = socket_.local_endpoint(e);
	if(e)
	{
		return std::string("");
	}

	boost::asio::ip::address address = endpoint.address();
	return address.to_string();
}

uint16 net_tcp_session_asio_impl::get_local_port() const
{
	if(!is_connect())
		return (uint16)-1;

	boost::system::error_code e;
	boost::asio::ip::tcp::endpoint endpoint = socket_.local_endpoint(e);
	if(e)
	{
		return (uint16)-1;
	}
	return endpoint.port();
}

void net_tcp_session_asio_impl::send_tgw(const std::string& ip, uint16 port)
{
}

bool net_tcp_session_asio_impl::send_packet(uint8* buff, uint32 length)
{
	if(!valid_ || closed_ || !length || !buff)
		return false;

	boost::recursive_mutex::scoped_lock autolock(send_queue_mutex_);

	if(!send_msg_buffer_)
		return false;

	msghead head;
	head.index_ = myself_msg_index_++;
	head.data_size_ = length;

	if(myself_msg_index_ >= 32767)
		myself_msg_index_ = 0;

	if(!send_msg_buffer_->write((const uint8*)&head, sizeof(msghead)) ||
		!send_msg_buffer_->write(buff, length))
	{
		close(std::string("send buffer full"), true);
		return false;
	}

	if(!sending_)
	{
		if(send_msg_buffer_->size() > 0)
		{
			sending_ = true;

			get_socket().get_io_service().post(boost::bind(&net_tcp_session_asio_impl::handle_session_write_msg_data, shared_from_this(),  
				boost::system::error_code(), 0));
		}
	}
	return true;
}

bool net_tcp_session_asio_impl::peek_packet(uint32& length)
{
	if(!valid_ || closed_)
		return false;

	boost::recursive_mutex::scoped_lock autolock(receive_queue_mutex_);

	if(!receive_msg_buffer_)
		return false;

	msghead head;

	do 
	{
		if(!receive_msg_buffer_->peek_read((uint8*)&head, sizeof(msghead)))
			return false;

		if(head.flags_ & msghead::MSG_FLAG_HEARBEAT)
		{
			if(!receive_msg_buffer_->read((uint8*)&head, sizeof(msghead)))
				return false;

			continue;
		}
		
		break;

	}while(true);

	if(head.data_size_ + sizeof(msghead) > receive_msg_buffer_->size())
	{
		length = 0;
		return false;
	}

	length = head.data_size_;
	return true;
}

int32 net_tcp_session_asio_impl::receive_packet(uint8* buff, uint32 bufflength)
{
	if(!valid_ || closed_ || !buff)
		return 0;

	boost::recursive_mutex::scoped_lock autolock(receive_queue_mutex_);

	uint32 msglength = 0;
	if(!peek_packet(msglength))
		return 0;

	if(msglength > bufflength)
		return 0;

	msghead head;
	if(!receive_msg_buffer_->read((uint8*)&head, sizeof(msghead)))
		return 0;

	if(!receive_msg_buffer_->read(buff, msglength))
		return 0;

	if(head.index_ != remote_msg_index_)
	{
		close("关闭连接, 非法消息index", true);
		return 0;
	}

	remote_msg_index_++;
	if(remote_msg_index_ >= 32767)
		remote_msg_index_ = 0;

	return msglength;
}

void net_tcp_session_asio_impl::close()
{
	close("进程主动关闭", false);
}

void net_tcp_session_asio_impl::close(const std::string& reason, bool force)
{
	boost::recursive_mutex::scoped_lock autolock(close_mutex_);

	if(closed_)
		return;

	last_error_info_ = reason;
	closed_ = true;
	
	if(force || !sending_)
	{// 忽略发送队列中的信息

		// 关闭socket
		valid_ = false;
		boost::system::error_code e;

		socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, e);
		socket_.close(e);
	}	
}

bool net_tcp_session_asio_impl::is_connect() const
{
	return is_valid();
}

bool net_tcp_session_asio_impl::is_valid() const
{
	return valid_;
}

void net_tcp_session_asio_impl::stop()
{
	close(std::string("stop"));
}

void net_tcp_session_asio_impl::start(uint32 min_sendbuff, uint32 max_sendbuff, uint32 min_receivebuff, uint32 max_receivebuff)
{
	if(send_msg_buffer_)
		return;

	if(receive_msg_buffer_)
		return;

	if(start_)
		return;

	start_ = true;

	last_msg_time_ = time(0);

	send_msg_buffer_ = boost::make_shared<circular_buffer>(min_sendbuff, max_sendbuff);
	receive_msg_buffer_ = boost::make_shared<circular_buffer>(min_receivebuff, max_receivebuff);

	boost::system::error_code ec;

	boost::asio::ip::tcp::no_delay nodelay(true);
	get_socket().set_option(nodelay, ec);

	valid_  = true;
	closed_ = false;

	boost::recursive_mutex::scoped_lock autolock(receive_queue_mutex_);

	boost::asio::async_read(get_socket(), boost::asio::buffer(receive_buffer_.data(), sizeof(datahead)), 
											boost::bind(&net_tcp_session_asio_impl::handle_session_read_msg_data, shared_from_this(), 
											true, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void net_tcp_session_asio_impl::handle_session_read_msg_data(bool readhead, const boost::system::error_code& e, std::size_t bytes_transferred)
{
	if(e)
	{
		close(e.message(), true);
	}
	else
	{
		boost::recursive_mutex::scoped_lock autolock(receive_queue_mutex_);

		if(!receive_msg_buffer_)
			return;

		last_msg_time_ = time(0);

		if(readhead)
		{
			const datahead *head = (const datahead*)receive_buffer_.data();

			if(head->mark_ != datahead::DATA_MARK_VALUE)
			{
				close(std::string("非法消息 mark_ 错误"), true);
				return;
			}

			if(head->data_size_ > NET_DATA_BUFFER_LENGTH)
			{
				close(std::string("非法消息 数据长度错误"), true);
				return;
			}

			boost::recursive_mutex::scoped_lock autolock(close_mutex_);
			if(!valid_)
				return;

			boost::asio::async_read(get_socket(), boost::asio::buffer(receive_buffer_.data() + sizeof(datahead), head->data_size_),
					boost::bind(&net_tcp_session_asio_impl::handle_session_read_msg_data, shared_from_this(),
						false, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
		}
		else
		{
			const datahead *head = (const datahead*)receive_buffer_.data();
			if(head->data_size_ != bytes_transferred)
			{
				close(std::string("非法消息 date_size != bytes_transferred"), true);
				return;
			}

			if(head->flags_ & datahead::DATA_FLAG_TGW)
			{
				// 不进行处理
			}
			else if(head->flags_ & datahead::DATA_FLAG_ZIP)
			{
				uLong uncompresslength = NET_DATA_BUFFER_LENGTH;
				if(Z_OK != uncompress(uncompress_buffer_.data(), &uncompresslength, receive_buffer_.data() + sizeof(datahead), bytes_transferred))
				{
					close(std::string("消息数据解压错误"), true);
					return;
				}
				
				if(!receive_msg_buffer_->write(uncompress_buffer_.data(), (uint32)uncompresslength))
				{
					close(std::string("receive buffer full"), true);
					return;
				}
			}
			else
			{
				if(!receive_msg_buffer_->write(receive_buffer_.data() + sizeof(datahead), bytes_transferred))
				{
					close(std::string("receive buffer full"), true);
					return;
				}
			}

			boost::recursive_mutex::scoped_lock autolock(close_mutex_);
			if(!valid_)
				return;

			boost::asio::async_read(get_socket(), boost::asio::buffer(receive_buffer_.data(), sizeof(datahead)), 
				boost::bind(&net_tcp_session_asio_impl::handle_session_read_msg_data, shared_from_this(), 
				true, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
		}
	}
}


void net_tcp_session_asio_impl::handle_session_write_msg_data(const boost::system::error_code& e, std::size_t bytes_transferred)
{
	if(e)
	{
		close(e.message(), true);
	}
	else
	{
		boost::recursive_mutex::scoped_lock autolock(send_queue_mutex_);
		
		if(!send_msg_buffer_)
			return;

		if(send_msg_buffer_->size() > 0)
		{
			datahead head;

			uint32 data_length = std::min(send_msg_buffer_->size(), (uint32)NET_DATA_BUFFER_LENGTH);

			if(!send_msg_buffer_->read(compress_buffer_.data(), data_length))
			{

			}
			
			/*uLong destlength = NET_DATA_BUFFER_LENGTH;
			if(Z_OK == compress(send_buffer_.data() + sizeof(datahead), &destlength, compress_buffer_.data(), data_length))
			{
				head.flags_ |= datahead::DATA_FLAG_ZIP;
				head.data_size_ = (uint16)destlength;
			}
			else*/
			{
				
				head.data_size_ = data_length;
				memcpy(send_buffer_.data() + sizeof(datahead), compress_buffer_.data(), data_length);
			}

			memcpy(send_buffer_.data(), &head, sizeof(datahead));

			boost::recursive_mutex::scoped_lock autolock(close_mutex_);
			if(!valid_)
				return;

			boost::asio::async_write(get_socket(), boost::asio::buffer(send_buffer_.data(), head.data_size_ + sizeof(datahead)), 
				boost::bind(&net_tcp_session_asio_impl::handle_session_write_msg_data, shared_from_this(),  
				boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
		}
		else
		{
			sending_ = false;

			if(closed_)
			{
				boost::recursive_mutex::scoped_lock autolock(close_mutex_);
				boost::system::error_code e;
				socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, e);
				socket_.close(e);
			}
		}
	}
}

std::string net_tcp_session_asio_impl::get_last_error() const
{
	return last_error_info_;
}

void net_tcp_session_asio_impl::tick(uint32 elapsed)
{
	if(!start_)
		return;

	if(time(0) - last_msg_time_ > NO_MSG_CLOSE_TIME)
	{
		close(std::string("心跳超时关闭"), true);
		return;
	}

	send_hearbeat();
}

void net_tcp_session_asio_impl::send_hearbeat()
{
	if(!valid_ || closed_ )
		return;

	boost::recursive_mutex::scoped_lock autolock(send_queue_mutex_);

	if(!send_msg_buffer_)
		return;

	msghead head;
	head.flags_ |= msghead::MSG_FLAG_HEARBEAT;
	head.data_size_ = 0;

	if(!send_msg_buffer_->write((const uint8*)&head, sizeof(msghead)))
	{
		close(std::string("send buffer full"), true);
		return;
	}

	if(!sending_)
	{
		if(send_msg_buffer_->size() > 0)
		{
			sending_ = true;

			get_socket().get_io_service().post(boost::bind(&net_tcp_session_asio_impl::handle_session_write_msg_data, shared_from_this(),  
				boost::system::error_code(), 0));
		}
	}
}

};