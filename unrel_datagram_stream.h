#ifndef UNREL_DATAGRAM_STREAM_H
#define UNREL_DATAGRAM_STREAM_H

#include "bytestream_base.h"
#include "packer.h"

class unrel_datagram_stream : public bytestream_base::listener, public unpacker
{
public:
	unrel_datagram_stream()
	{
		bytestream = NULL;
	}
	virtual ~unrel_datagram_stream(){}
	
	void set_bytestream(bytestream_base *b)
	{
		b->set_listener(NULL);
		bytestream = b;
		b->set_listener(this);
	}
	
	int send(uint8_t *data, size_t size)
	{
		if(bytestream == NULL)
		{
			return -1;
		}
		
		package_ptr p(data, size);
		
		size_t pack_size = sizeof(package_header) + size + sizeof(p.checksum);
		char *buf = (char *)malloc(pack_size);

		memcpy(buf, &p, sizeof(package_header));
		memcpy(buf + sizeof(package_header), data, size);
		memcpy(buf + sizeof(package_header) + size, &p.checksum, sizeof(p.checksum));
		
		bytestream->send((uint8_t *)buf, pack_size);
		free(buf);
		
		return 0;
	}
										   

private:
	bytestream_base *bytestream;
	
	void bytes_arrived()
	{
		push_data((char *)bytestream->get_data(), bytestream->get_size());
		bytestream->clear_data();
	}
};

#endif // UNREL_DATAGRAM_STREAM_H
