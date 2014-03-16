#ifndef __net_h__
#define __net_h__

#include "def.h"

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/make_shared.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/lexical_cast.hpp>

#include <string>
#include <list>
#include <vector>
#include <set>
#include <map>
#include <queue>

namespace net
{
#ifdef NDEBUG
#define NO_MSG_CLOSE_TIME 180
#else
#define NO_MSG_CLOSE_TIME 600
#endif


#define DEFAULT_RECEIVE_BUFFER_LENGTH (1024 * 64)
#define DEFAULT_SEND_BUFFER_LENGTH	(1024 * 64)

#define MAX_SEND_LENGTH (4096)

struct net_tcp_session_interface
{
	virtual std::string get_remote_address() const					= 0;	// ȡԶ�˵�IP��ַ
	virtual uint16 get_remote_port() const							= 0;	// ȡԶ�˵Ķ˿ں�
	virtual std::string get_local_address() const					= 0;	// ȡ�Լ���IP��ַ
	virtual uint16 get_local_port() const							= 0;	// ȡ�Լ��Ķ˿ں�
	virtual bool send_packet(uint8* buff, uint32 length)			= 0;	// ����һ�����ݰ�
	virtual bool peek_packet(uint32& length)						= 0;	// �鿴�Ƿ����һ����Ϣ��, �������ȡ�ó���
	virtual int32 receive_packet(uint8* buff, uint32 bufflength)	= 0;	// ����һ�����ݰ�, ��������Ϣ���ĳ���
	virtual void close()											= 0;	// �����ر�����, �����������ݲ��ܷ���
	virtual bool is_connect() const									= 0;	// �Ƿ�����
	virtual bool is_valid()	const									= 0;	// 
	virtual void start(uint32 min_sendbuff = DEFAULT_SEND_BUFFER_LENGTH, uint32 max_sendbuff = DEFAULT_SEND_BUFFER_LENGTH, uint32 min_receivebuff = DEFAULT_RECEIVE_BUFFER_LENGTH, uint32 max_receivebuff = DEFAULT_RECEIVE_BUFFER_LENGTH)= 0;	// ��ʹִ�������߼�
	virtual void stop()												= 0;	// ִֹͣ�������߼�, ���������ڵ����ݷ�����ɺ�ر���������
	virtual std::string get_last_error() const						= 0;	// ȡ�ùر������ԭ��								
	virtual void tick(uint32 elapsed)								= 0;

	virtual void send_tgw(const std::string& ip, uint16 port)		= 0;
};

typedef boost::shared_ptr<net_tcp_session_interface> session_ptr;

struct net_tcp_session_mgr_interface
{
	//virtual bool remove_session(session_ptr session) = 0;
	virtual session_ptr get_newsession() = 0; // �����첽(�ո����������)
};

/////////////////////////////////////////////////////����˽ӿڶ���//////////////////////////////////////
struct net_tcp_server_interface : public net_tcp_session_mgr_interface
{
	virtual bool listen(const std::string& ipv4, uint16 port) = 0;
};

/////////////////////////////////////////////////////�ͻ��˽ӿڶ���//////////////////////////////////////
struct net_tcp_client_interface : public net_tcp_session_mgr_interface
{
	virtual bool async_connect(const std::string& ip, uint16 port) = 0;
	virtual session_ptr connect(const std::string& ipv4, uint16 port) = 0;
};

typedef boost::shared_ptr<net_tcp_server_interface> tcp_server_ptr;
typedef boost::shared_ptr<net_tcp_client_interface> tcp_client_ptr;

// ������Ӧ��ʵ��
bool net_init(uint16 threadcnt);
void net_clear();

tcp_server_ptr create_tcp_server(uint32 min_sendbuff = DEFAULT_SEND_BUFFER_LENGTH, uint32 max_sendbuff = DEFAULT_SEND_BUFFER_LENGTH, uint32 min_receivebuff = DEFAULT_RECEIVE_BUFFER_LENGTH, uint32 max_receivebuff = DEFAULT_RECEIVE_BUFFER_LENGTH);
tcp_client_ptr creat_tcp_client(uint32 min_sendbuff = DEFAULT_SEND_BUFFER_LENGTH, uint32 max_sendbuff = DEFAULT_SEND_BUFFER_LENGTH, uint32 min_receivebuff = DEFAULT_RECEIVE_BUFFER_LENGTH, uint32 max_receivebuff = DEFAULT_RECEIVE_BUFFER_LENGTH);

};
#endif
