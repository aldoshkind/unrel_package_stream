#ifndef PACKER_H
#define PACKER_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "slip.h"

#if (__cplusplus >= 201103L) && !defined(ARDUINO)
#define DESKTOP_MODE
#endif

#ifdef DESKTOP_MODE
#include <limits>
#endif

#if !(__cplusplus > 201103L)
#define nullptr NULL
#endif

typedef uint8_t checksum_t;

static checksum_t get_checksum(void *data, size_t size, checksum_t initial_value = 0)
{
	checksum_t cs = initial_value;
	for (size_t i = 0; i < size; i += 1)
	{
		uint8_t byte = *((uint8_t *)data + i);
		cs ^= byte;
		cs += 1;
	}

	return cs;
}

static const int package_magic = 0x55;
static const int max_package_size = 64;

struct __attribute__((packed)) package_header
{
	uint8_t magic;
	uint8_t package_size;
	checksum_t checksum;

	package_header()
	{
		magic = package_magic;
		package_size = sizeof(package_header);
		checksum = 0;
	}
	
	void update_checksum()
	{
		checksum = get_checksum((uint8_t *) this, sizeof(package_header) - 1);
	}
};

struct __attribute__((packed)) package_ptr : public package_header
{
	uint8_t *data_ptr;
	size_t data_size;
	checksum_t checksum;
	package_ptr(uint8_t *data, size_t size)
	{
		data_ptr = data;
		data_size = size;
		
		package_size = sizeof(package_header) + size + 1;
		
		update_checksum();
	}
	
	void update_checksum()
	{
		package_header::update_checksum();
		checksum_t pl_checksum = get_checksum((uint8_t *) (this), sizeof(package_header), package_header::checksum);
		checksum = get_checksum(data_ptr, data_size, pl_checksum);
	}
};

template<class payload_type>
struct __attribute__((packed)) package
{
	package_header header;
	payload_type payload;
	checksum_t checksum;

#ifdef DESKTOP_MODE
	static_assert(sizeof(payload_type)
					  < std::numeric_limits<decltype(package_header::package_size)>::max() - sizeof(package_header) - sizeof(checksum),
				  "payload too big");

#endif

	package()
	{
		header.package_size = sizeof(*this);
		update_checksum();
	}
	
	package(payload_type p) : payload(p)
	{
		header.package_size = sizeof(*this);
		update_checksum();
	}

	void update_checksum()
	{
		header.update_checksum();
		checksum = get_checksum((uint8_t *) (this), sizeof(*this) - 1, header.checksum);
	}
};

class unpacker
{
public:
	unpacker()
	{
		//
	}

	virtual ~unpacker(){}
	
	void push_data(char *data, size_t size)
	{
		for (int i = 0; i < size; i += 1)
		{
			push_byte(data[i]);
		}
	}

	slip::slip<max_package_size> framer;	
private:
	void package_ready(package_header *p)
	{
		datagram_arrived((uint8_t *)&((package<char> *)p)->payload, p->package_size);
	}
	
public:
	template<class t>
	size_t sufficient_size()
	{
		return slip::sufficient_buffer_size<t>();
	}
	
	size_t encode(const void *src, size_t srclen, void *dst, size_t dstlen)
	{
		return framer.encode_packet(src, srclen, dst, dstlen);
	}
	
private:
	virtual void datagram_arrived(uint8_t *data, size_t size) = 0;

	void push_byte(char b)
	{
		framer.push_byte(b);
		if(framer.is_package_ready() == false)
		{
			return;
		}
		
		size_t size = framer.get_data_size();
		if(size < sizeof(package_header))
		{
			return;
		}
		
		package_header *h = (package_header *)framer.get_data();
		checksum_t header_checksum_calculated = get_checksum(h, sizeof(package_header) - 1);
		checksum_t header_checksum_received = h->checksum;		
		if(header_checksum_received != header_checksum_calculated)
		{
			return;			
		}
		
		if(size != h->package_size)
		{
			return;			
		}
		
		checksum_t package_checksum_calculated = get_checksum(h, size - sizeof (checksum_t), h->checksum);
		checksum_t package_checksum_received = *(checksum_t *)(((char *)h) + size - sizeof (checksum_t));
		
		if(package_checksum_received != package_checksum_calculated)
		{
			return;			
		}
		
		package_ready(h);
	}
};

#endif // PACKER_H
