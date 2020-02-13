#pragma once

#include <stdlib.h>

namespace slip
{

typedef unsigned char uchar;

static const uchar symbol_end = 0xC0;    /* indicates end of packet */
static const uchar symbol_esc = 0xDB;    /* indicates byte stuffing */
static const uchar symbol_esc_end = 0xDC;    /* ESC ESC_END means END data byte */
static const uchar symbol_esc_esc = 0xDE;    /* ESC ESC_ESC means ESC data byte */


template <class type>
size_t sufficient_buffer_size()
{
	return sizeof(type) * 2 + 2;
}

#define SUFFICIENT_BUFFER_SIZE(t) (sizeof(type) * 2 + 2)

template <size_t max_package_size>
class slip
{
public:
	slip() {}
	~slip() {}

	uchar encode_byte(uchar val, void *dst)
	{
		switch(val)
		{
		case symbol_end:
			((uchar *)dst)[0] = symbol_esc;
			((uchar *)dst)[1] = symbol_esc_end;
			return 2;
			break;
		case symbol_esc:
			((uchar *)dst)[0] = symbol_esc;
			((uchar *)dst)[1] = symbol_esc_esc;
			return 2;
			break;
		default:
			((uchar *)dst)[0] = val;
			return 1;
		}
		return 0;
	}

    size_t encode_packet(const void *src, size_t srclen, void *dst, size_t dstlen)
	{
		uchar *buf = (uchar *)src;
		uchar *dst_buf = (uchar *)dst;
		size_t dst_pos = 0;
		
		if((dstlen < (2 + 2 * srclen)) || (dstlen < 1))
		{
			//the esc chars will change the size a bit but who knows exacly. in hardwareserial for uno sz==64 --> 31 is the max sz of tx pkt to be safe.
			return false;
		}
		
		dst_buf[dst_pos++] = symbol_end;
	
		for(size_t i = 0 ; i < srclen ; i += 1)
		{
			uchar val = buf[i];
			dst_pos += encode_byte(val, dst_buf + dst_pos);
		}
		dst_buf[dst_pos++] = symbol_end;
		return dst_pos;
	}
	
	void push_data(void *data, size_t size)
	{
		for (int i = 0; i < size; i += 1)
		{
			push_byte(((uchar *)data)[i]);
		}
	}
	
	void push_byte(uchar c)
	{
		package_size = 0;
		if(c == symbol_end)
		{
			if(inbuffer_ptr > 0)
			{
				//slipReadCallback(inbuffer, inbuffer_ptr);
				// package ready
				package_size = inbuffer_ptr;
			}
			inbuffer_ptr = 0;
			lastbyte = c;
			return;
		}
	
		if(inbuffer_ptr >= max_package_size)
		{
			// overflow
			inbuffer_ptr = 0;
		}
	
		if(lastbyte == symbol_esc)
		{
			switch(c)
			{
			case symbol_esc_end:
				inbuffer[inbuffer_ptr] = symbol_end;
				inbuffer_ptr++;
				break;
			case symbol_esc_esc:
				inbuffer[inbuffer_ptr] = symbol_esc;
				inbuffer_ptr++;
				break;
			}
			lastbyte = c;
			return;
		}
	
		if(c != symbol_esc)
		{
			inbuffer[inbuffer_ptr] = c;
			inbuffer_ptr++;
		}
	
		lastbyte = c;
	}
	
	bool is_package_ready()
	{
		return package_size > 0;
	}
	
	void *get_data()
	{
		return inbuffer;
	}
	
	size_t get_data_size()
	{
		return package_size;
	}
	
private:
	uchar lastbyte = symbol_end;
	uchar inbuffer[max_package_size] = {0};
    size_t inbuffer_ptr = 0;
	size_t package_size = 0;
};

}
