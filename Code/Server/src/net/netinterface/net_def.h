#ifndef __net_def_h__
#define __net_def_h__

#include "def.h"

namespace net
{

#pragma pack(push, 1)

struct datahead
{
	static const uint16 DATA_MARK_VALUE = 0xAA;
	
	enum 
	{
		DATA_FLAG_ZIP		= 0x1,	
		DATA_FLAG_ENCRYPT	= 0x2, 

		DATA_FLAG_TGW		= 0x4,
	};

	uint8	mark_;
	uint8	flags_;
	uint16  data_size_;

	datahead()
	{
		mark_	= DATA_MARK_VALUE;
		flags_	= 0;
		data_size_	= 0;
	}
};


struct msghead
{
	enum
	{
		MSG_FLAG_HEARBEAT = 0x1,
	};

	uint16 index_;			// ��Ϣ���к�
	uint16 flags_;			// ��Ϣ�İ汾��
	uint32 data_size_;		// ����Ϣ������ĳ���

	msghead()
	{
		index_ = 0;
		flags_ = 0;
		data_size_ = 0;
	}
};

#pragma pack(pop)

#define NET_MSGHEAD_SIZE (sizeof(msghead))

};

#endif