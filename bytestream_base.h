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
		virtual void device_opened() = 0;
		virtual void device_closed() = 0;
	};
	
	bytestream_base()
	{
		l = NULL;
	}
	
	virtual ~bytestream_base()
	{
		//
	}
	
	void set_listener(listener *l)
	{
		this->l = l;
		if(is_connected() && l != NULL)
		{
			l->device_opened();
		}
	}
	
	virtual void *get_data() = 0;
	virtual size_t get_size() = 0;
	virtual void clear_data() = 0;
	
	virtual int send(void *data, size_t size) = 0;
	
protected:
	void notify_data_arrived()
	{
		if(l != NULL)
		{
			l->bytes_arrived();
		}
	}
	
	void notify_connected()
	{
		if(l != NULL)
		{
			l->device_opened();
		}
	}
	
	void notify_disconnected()
	{
		if(l != NULL)
		{
			l->device_closed();
		}
	}
	
	virtual bool is_connected()	= 0;
	
private:
	listener *l;
};

#endif // BYTESTREAM_BASE_H
