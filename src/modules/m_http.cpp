/*       +------------------------------------+
 *       | Inspire Internet Relay Chat Daemon |
 *       +------------------------------------+
 *
 *  InspIRCd is copyright (C) 2002-2006 ChatSpike-Dev.
 *		       E-mail:
 *		<brain@chatspike.net>
 *	   	  <Craig@chatspike.net>
 *     
 * Written by Craig Edwards, Craig McLure, and others.
 * This program is free but copyrighted software; see
 *	    the file COPYING for details.
 *
 * ---------------------------------------------------
 */

using namespace std;

#include <stdio.h>
#include "users.h"
#include "channels.h"
#include "modules.h"
#include "inspsocket.h"
#include "helperfuncs.h"

/* $ModDesc: Provides HTTP serving facilities to modules */

static Server *Srv;

enum HttpState
{
	HTTP_LISTEN = 0,
	HTTP_SERVE_WAIT_REQUEST = 1,
	HTTP_SERVE_SEND_DATA = 2
};

class HttpSocket : public InspSocket
{
	FileReader* index;
	HttpState InternalState;
	std::stringstream headers;

 public:

	HttpSocket(std::string host, int port, bool listening, unsigned long maxtime, FileReader* index_page) : InspSocket(host, port, listening, maxtime), index(index_page)
	{
		log(DEBUG,"HttpSocket constructor");
		InternalState = HTTP_LISTEN;
	}

	HttpSocket(int newfd, char* ip) : InspSocket(newfd, ip)
	{
		InternalState = HTTP_SERVE_WAIT_REQUEST;
	}

	virtual int OnIncomingConnection(int newsock, char* ip)
	{
		if (InternalState == HTTP_LISTEN)
		{
			HttpSocket* s = new HttpSocket(newsock, ip);
			Srv->AddSocket(s);
		}
		return true;
	}

	virtual void OnClose()
	{
	}

	virtual bool OnDataReady()
	{
		char* data = this->Read();
		/* Check that the data read is a valid pointer and it has some content */
		if (data && *data)
		{
			headers << data;

			if (headers.str().find("\r\n\r\n"))
			{
				/* Headers are complete */
				InternalState = HTTP_SERVE_SEND_DATA;
				this->Write("<HTML><H1>COWS.</H1></HTML>");
			}
		}
		return true;
	}
};

class ModuleHttp : public Module
{
	int port;
	std::string host;
	std::string bindip;
	std::string indexfile;

	FileReader index;

	HttpSocket* http;

 public:

	void ReadConfig()
	{
		ConfigReader c;
		this->host = c.ReadValue("http", "host", 0);
		this->bindip = c.ReadValue("http", "ip", 0);
		this->port = c.ReadInteger("http", "port", 0, true);
		this->indexfile = c.ReadValue("http", "index", 0);

		index.LoadFile(this->indexfile);
	}

	void CreateListener()
	{
		http = new HttpSocket(this->bindip, this->port, true, 0, &index);
		Srv->AddSocket(http);
	}

	ModuleHttp(Server* Me) : Module::Module(Me)
	{
		ReadConfig();
		CreateListener();
	}

	void Implements(char* List)
	{
		List[I_OnEvent] = List[I_OnRequest] = 1;
	}

	virtual ~ModuleHttp()
	{
		Srv->DelSocket(http);
	}

	virtual Version GetVersion()
	{
		return Version(1,0,0,0,VF_STATIC|VF_VENDOR);
	}
};


class ModuleHttpFactory : public ModuleFactory
{
 public:
	ModuleHttpFactory()
	{
	}
	
	~ModuleHttpFactory()
	{
	}
	
	virtual Module * CreateModule(Server* Me)
	{
		return new ModuleHttp(Me);
	}
};


extern "C" void * init_module( void )
{
	return new ModuleHttpFactory;
}
