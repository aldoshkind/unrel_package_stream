#ifndef UNREL_DATAGRAM_STREAM_H
#define UNREL_DATAGRAM_STREAM_H

#include "bytestream_base.h"
#include "unrel_package_stream/packer.h"

class unrel_datagram_stream : public bytestream_base::listener, public unpacker
{
public:
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
		
		bytestream->send((uint8_t *)&p, sizeof(package_header));
		bytestream->send(data, size);
		bytestream->send((uint8_t *)&p.checksum, sizeof(p.checksum));
		
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
