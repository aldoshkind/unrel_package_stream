#ifndef PACKER_H
#define PACKER_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if (__cplusplus >= 201103L)
#include <limits>
#endif

#if !(__cplusplus > 201103L)
#define nullptr NULL
#endif

typedef uint8_t checksum_t;

static checksum_t get_checksum(uint8_t *data, size_t size, checksum_t initial_value = 0)
{
	checksum_t cs = initial_value;
	for (size_t i = 0; i < size; i += 1)
	{
		uint8_t byte = *((uint8_t *) data + i);
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

#if (__cplusplus >= 201103L)
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
		memset(buffer, 0, sizeof(buffer));
		start_pos = -1;
		lookup_start_pos = 0;
		push_pos = 0;
		header_ptr = NULL;
	}

	virtual ~unpacker(){}
	
	void push_data(char *data, uint8_t size)
	{
		for (int i = 0; i < size; i += 1)
		{
			push_byte(data[i]);
		}
	}

private:
	char buffer[max_package_size * 2];

	int start_pos;
	int lookup_start_pos;
	int push_pos;

	package_header *header_ptr;

	int available()
	{
		int ppos = ((push_pos > start_pos) ? push_pos : (push_pos + max_package_size));
		int shift = ppos - start_pos;
		return shift;
	}

	int find_start(int read_pos)
	{
		int start = -1;
		for (int i = read_pos; i != push_pos; i = (i + 1) % max_package_size)
		{
			// wait for package start
			if (buffer[i] == package_magic)
			{
				start = i;
				//printf("start at %d\n", start);
				break;
			}
		}
		return start;
	}

	void package_ready(package_header *p)
	{
		datagram_arrived((uint8_t *)&((package<char> *)p)->payload, p->package_size);
	}
	
	virtual void datagram_arrived(uint8_t *data, size_t size) = 0;

	void push_byte(char b)
	{
		buffer[push_pos] = b;
		buffer[push_pos + max_package_size] = b;
		push_pos = (push_pos + 1) % max_package_size;

		for (;;)
		{
			if (start_pos == -1)
			{
				start_pos = find_start(lookup_start_pos);
				if (start_pos == -1)
				{
					lookup_start_pos = push_pos;
					return;
				}
				lookup_start_pos = start_pos + 1;
			}

			if (header_ptr == nullptr)
			{
				if (available() >= (int)sizeof(package_header))
				{
					checksum_t calculated = get_checksum((uint8_t *) &buffer[start_pos], sizeof(package_header) - 1);
					header_ptr = (package_header *) &buffer[start_pos];
					checksum_t received = header_ptr->checksum;

					if ((calculated != received) || (header_ptr->package_size < (int)sizeof(package_header)) || (header_ptr->package_size > max_package_size))
					{
						//printf("wrong header crc\n");
						lookup_start_pos = (start_pos + 1) % max_package_size;
						header_ptr = nullptr;
						start_pos = -1;
						continue;
					}
					else
					{
						//printf("header ok\n");
					}
				}
				else
				{
					//printf("not enough data for header\n");
					return;
				}
			}

			if (header_ptr != nullptr)
			{
				if (available() >= header_ptr->package_size)
				{
					int end_pos = start_pos + header_ptr->package_size;
                                        // если проскочил некорректный заголовок - сбрасываем состояние
                                        if((header_ptr->package_size < (int)sizeof(package_header)) || (header_ptr->package_size > max_package_size))
                                        {
                                            lookup_start_pos = (start_pos + 1) % max_package_size;
                                            header_ptr = nullptr;
                                            start_pos = -1;
                                            return;
                                        }
                                        uint8_t size = header_ptr->package_size - 1;
					checksum_t calculated = get_checksum((uint8_t *) &buffer[start_pos], size, header_ptr->checksum);
					checksum_t received = buffer[end_pos - 1];

					if (calculated == received)
					{
						//printf("package\n");
						lookup_start_pos = (start_pos + 1) % max_package_size;
						package_ready((package_header *) &buffer[start_pos]);
					}
					else
					{
						//printf("wrong body crc\n");
						lookup_start_pos = (start_pos + 1) % max_package_size;
					}
					header_ptr = nullptr;
					start_pos = -1;
				}
				else
				{
					//printf("not enough data for body: %d/%d\n", available(), header_ptr->package_size);
					return;
				}
			}
		}
	}
};

#endif // PACKER_H
