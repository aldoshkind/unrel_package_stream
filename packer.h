#pragma once

#include <stdint.h>
#include <stdlib.h>

//#include <limits>

#if !(__cplusplus > 199711L)
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
	uint8_t magic = package_magic;
	uint8_t system = 0;
	uint8_t op = 0;
	uint8_t package_size = sizeof(package_header);
	checksum_t checksum = 0;

	void update_checksum()
	{
		checksum = get_checksum((uint8_t *) this, sizeof(package_header) - 1);
	}
};

template<class payload_type, uint8_t sys, uint8_t operation>
struct __attribute__((packed)) package
{
	package_header header;
	payload_type payload;
	checksum_t checksum = 0;

#if __cplusplus > 199711L
	static_assert(sizeof(payload_type)
					  < std::numeric_limits<decltype(package_header::package_size)>::max() - sizeof(package_header) - sizeof(checksum),
				  "payload too big");

#endif

	package()
	{
		header.system = sys;
		header.op = operation;
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
	void push_data(char *data, uint8_t size)
	{
		for (int i = 0; i < size; i += 1)
		{
			push_byte(data[i]);
		}
	}

private:
	char buffer[max_package_size * 2] = {0};

	int start_pos = -1;
	int lookup_start_pos = 0;
	int push_pos = 0;

	package_header *header_ptr = nullptr;

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

	virtual void package_ready(package_header *p)
	{
		//printf("package\n");
	}

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
				if (available() >= sizeof(package_header))
				{
					checksum_t calculated = get_checksum((uint8_t *) &buffer[start_pos], sizeof(package_header) - 1);
					header_ptr = (package_header *) &buffer[start_pos];
					checksum_t received = header_ptr->checksum;

					if (calculated != received)
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
					checksum_t calculated = get_checksum((uint8_t *) &buffer[start_pos], (uint8_t) header_ptr->package_size - 1, header_ptr->checksum);
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
