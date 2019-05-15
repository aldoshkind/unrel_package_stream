#ifndef BYTESTREAM_BASE_H
#define BYTESTREAM_BASE_H

#include <stdint.h>
#include <stdlib.h>

class bytestream_base
{
public:
	class listener
	{
	public:
		virtual ~listener(){}
		virtual void bytes_arrived() = 0;
	};
	
	virtual ~bytestream_base()
	{
		//
	}
	
	void set_listener(listener *l)
	{
		this->l = l;
	}
	
	virtual uint8_t *get_data() = 0;
	virtual size_t get_size() = 0;
	
	virtual int send(uint8_t *data, size_t size) = 0;
	
protected:
	void notify_data_arrived()
	{
		l->bytes_arrived();
	}
	
private:
	listener *l;
};

#endif // BYTESTREAM_BASE_H
