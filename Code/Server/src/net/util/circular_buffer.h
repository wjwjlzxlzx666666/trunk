
#ifndef __circular_buffer_h__
#define __circular_buffer_h__

#include "def.h"
#include <vector>

namespace net
{
class circular_buffer
{
public:
	circular_buffer(uint32 minlength, uint32 maxlength) : max_length_(maxlength), head_(0), tail_(0), buffer_(minlength)
	{
	}

	~circular_buffer()
	{
	}

public:
	bool write(const uint8* buffer, uint32 write_length)
	{
		if(capacity() < write_length + 1 && !increase(write_length + 1 - capacity()))
			return false;

		if(head_ <= tail_)
		{
			uint32 tail_capacity = buffer_.size() - tail_;
			if(tail_capacity < write_length)
			{
				memcpy(&buffer_[tail_], buffer, tail_capacity);
				tail_ = (tail_ + tail_capacity) % buffer_.size();

				memcpy(&buffer_[tail_], buffer + tail_capacity, write_length - tail_capacity);
				tail_ = (tail_ + write_length - tail_capacity) % buffer_.size();

				return true;
			}
			else
			{
				memcpy(&buffer_[tail_], buffer, write_length);
				tail_ = (tail_ + write_length) % buffer_.size();

				return true;
			}
		}
		else
		{
			memcpy(&buffer_[tail_], buffer, write_length);
			tail_ = (tail_ + write_length) % buffer_.size();

			return true;
		}
	}

	bool read(uint8 *buffer, uint32 read_length)
	{
		if(size() < read_length)
			return false;

		if(head_ < tail_)
		{
			memcpy(buffer, &buffer_[head_], read_length);
			head_ = (head_ + read_length) % buffer_.size();

			return true;
		}
		else
		{
			uint32 length = buffer_.size() - head_;
			if(length < read_length)
			{
				memcpy(buffer, &buffer_[head_], length);
				head_ = (head_ + length) % buffer_.size();

				memcpy(buffer + length, &buffer_[head_], read_length - length);
				head_ = (head_ + read_length - length) % buffer_.size();

				return true;
			}
			else
			{
				memcpy(buffer, &buffer_[head_], read_length);
				head_ = (head_ + read_length) % buffer_.size();

				return true;
			}
		}
	}

	bool peek_read(uint8 *buffer, uint32 read_length)
	{
		uint32 head_pos = head_;
		bool rst = read(buffer, read_length);
		head_ = head_pos;

		return rst;
	}

	uint32 size() const
	{
		if(tail_ >= head_)
			return tail_ - head_;

		return tail_ + buffer_.size() - head_;
	}

	uint32 max_size() const
	{
		return max_length_;
	}

private:
	uint32 capacity() const
	{
		return buffer_.size() - size();
	}

	bool increase(uint32 size)
	{
		if(size + buffer_.size() > max_size())
			return false;

		uint32 target_length = buffer_.size() + size;
		target_length += target_length;
		if(target_length > max_size())
			target_length = max_size();

		std::size_t oldsize = buffer_.size();
		buffer_.resize(target_length);

		if(head_ > tail_)
		{
			std::size_t length = oldsize - head_;

			std::size_t newhead = buffer_.size() - length;
			memmove(&buffer_[newhead], &buffer_[head_], length);

			head_ = newhead;
		}
		
		return true;
	}

private:
	std::vector<uint8> buffer_;
	uint32			   max_length_;

	uint32 head_;
	uint32 tail_;
};

};

#endif