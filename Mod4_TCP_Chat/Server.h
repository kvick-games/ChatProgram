#pragma once

#include "AppBase.h"

#include <vector>

//TODO: Eventually get rid of this
#include "Globals.h"

class Server : public AppBase
{
public:
	Server();
	~Server();

	int Init();

	void Run();
	void CheckForClientConnections();
	void CheckForClientMessages();

	void Broadcast(std::string message, std::string from, Connection* pFilteredConnection);

	fd_set MakeCopySet();

private:
	Connection* FindConnection(SOCKET sock);

public:
	
	SOCKET m_serverSocket;

	std::vector<Connection>	m_clientConnections;

	constexpr static int k_bufferSize = 256;
	char m_buffer[k_bufferSize];
};