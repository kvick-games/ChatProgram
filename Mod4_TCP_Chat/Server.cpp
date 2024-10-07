#include "Server.h"

#include "Globals.h"

Server::Server()
{
	
}

Server::~Server()
{
	//Cleanup
	shutdown(m_serverSocket, SD_BOTH);
	closesocket(m_serverSocket);
}

int Server::Init()
{
	LOG_SERVER("Starting up");
	m_serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == m_serverSocket)
	{
		ERR("Can't create server socket, Err #" << WSAGetLastError());
		return -1;
	}

	m_serverPort = AskUserForPort();

	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(m_serverPort);
	inet_pton(addr.sin_family, "127.0.0.1", &addr.sin_addr);

	//Bind
	LOG_SERVER("Binding to port " << m_serverPort << "...");
	if (SOCKET_ERROR == bind(m_serverSocket, (sockaddr*)&addr, sizeof(addr)))
	{
		ERR("Can't bind to port " << m_serverPort << ", Err #" << WSAGetLastError());
		return -1;
	}
	LOG_SERVER("Binded to port " << m_serverPort);

	//Listen
	LOG_SERVER("Listening for connections...");
	constexpr int k_backlog = 5;
	if (SOCKET_ERROR == listen(m_serverSocket, k_backlog))
	{
		ERR("Can't listen for connections, Err #" << WSAGetLastError());
		return -1;
	}

	return 0;
}

std::string IpToString(sockaddr_in& addr)
{
	char ipStr[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &addr.sin_addr, ipStr, INET_ADDRSTRLEN);
	return std::string(ipStr);
}

void Server::CheckForClientConnections()
{
#if EXTRA_LOGGING == 1
	LOG_SERVER("Checking for client connections...");
#endif
	
	sockaddr_in clientAddr = {};
	int clientAddrLen = sizeof(clientAddr);

	struct timeval timeout;
	timeout.tv_sec = 1; // 1 second timeout
	timeout.tv_usec = 0;

	fd_set copySet;
	FD_ZERO(&copySet);
	FD_SET(m_serverSocket, &copySet);

	int socketCount = select(0, &copySet, nullptr, nullptr, &timeout);
	if (socketCount == SOCKET_ERROR)
	{
		ERR("Can't select socket, Err #" << WSAGetLastError());
		return;
	}

	if (socketCount == 0)
	{
		return;
	}

	//Accept new client connection(s)
	SOCKET clientSock = accept(m_serverSocket, (sockaddr*)(&clientAddr), &clientAddrLen);
	if (INVALID_SOCKET == clientSock)
	{
		ERR("Can't accept client connection, Err #" << WSAGetLastError());
		return;
	}

	//get initial info from client
	int bytesRecv = recv(clientSock, &m_buffer[0], k_bufferSize, 0);
	std::string clientName;
	if (bytesRecv > 0)
	{
		std::string message = m_buffer;
		clientName = message;
		LOG_SERVER("Received from client: " << message);
	}
	else if (bytesRecv == 0)
	{
		Connection* pConnection = FindConnection(clientSock);
		if (pConnection == nullptr)
		{
			ERR("Client disconnected before sending name");
			Cleanup();
			return;
		}
		
		std::string userID = pConnection->publicInfo.ConstructUserDisplayName();
		ERR("Client disconnected: " + userID);
		Broadcast(userID + " has left!", "Server", pConnection);
		closesocket(clientSock);
		return;
	}
	else if (bytesRecv == SOCKET_ERROR)
	{
		ERR("Error receiving data from client, Err #" << WSAGetLastError());

		Connection* pConnection = FindConnection(clientSock);
		if (pConnection == nullptr)
		{
			ERR("Client disconnected before sending name");
			Cleanup();
			return;
		}

		std::string userID = pConnection->publicInfo.ConstructUserDisplayName();
		ERR("Client disconnected: " + userID);
		Broadcast(userID + " has left!", "Server", pConnection);

		closesocket(clientSock);
		return;
	}

	Connection newConnection;
	newConnection.sock = clientSock;
	newConnection.addr = clientAddr;
	ClientPublicInfo publicInfo;
	publicInfo.ip = IpToString(clientAddr);
	publicInfo.name = clientName;
	publicInfo.port = std::to_string(ntohs(clientAddr.sin_port));
	newConnection.publicInfo = publicInfo;
	m_clientConnections.push_back(newConnection);

	//Announce new client
	std::string announceMsg = publicInfo.ConstructUserDisplayName() + " has joined!";
	LOG_SERVER("New Client " << announceMsg);

	//send some data to the client
	Broadcast(announceMsg, "Server", &newConnection);

}

Connection* Server::FindConnection(SOCKET sock)
{
	for (Connection& connection : m_clientConnections)
	{
		if (connection.sock == sock)
		{
			return &connection;
		}
	}
	return nullptr;
}

void Server::CheckForClientMessages()
{
#if EXTRA_LOGGING == 1
	LOG_SERVER("Checking for client messages...");
#endif

	for (int iClientConn = int(m_clientConnections.size()-1); iClientConn >= 0; --iClientConn)
	{
		Connection& connection = m_clientConnections[iClientConn];

		//Set socket to non-blocking mode
		u_long k_nonBlockingMode = 1;
		ioctlsocket(connection.sock, FIONBIO, &k_nonBlockingMode);

		//Check if the client has sent any data
		char localBuffer[k_maxClientSendSize];
		ZeroMemory(localBuffer, k_maxClientSendSize);

		int bytesRecv = recv(connection.sock, &localBuffer[0], k_maxClientSendSize, 0);
		if (bytesRecv > 0)
		{
#if EXTRA_LOGGING == 1
			LOG_SERVER("Received: " << bytesRecv << " bytes");
#endif

			bool messageIsComplete = false;
			size_t newlinePos = -1;
			newlinePos = std::string(localBuffer).find('\n');

			if (connection.currentBufferEnd >= k_serverConnectionBufferSize)
			{
				ERR("Message buffer overflow, try message again.");
				connection.ClearBuffer();
				continue;
			}

			//Check if the message is complete
			if (newlinePos != std::string::npos)
			{
				messageIsComplete = true;
			}

			//LOG_SERVER("Received from client: " << m_buffer);
			std::string message;
			//Append the size of the local buffer to the message
			message.append(localBuffer, bytesRecv);

			//Prepend the client's ip to the message
			std::string userLabel = connection.publicInfo.ConstructUserDisplayName();
			std::string partial = messageIsComplete ? "" : "partial ";
			LOG_SERVER("Received " + partial + "from client (" + userLabel + ") : " << message);

			//Copy the contents of the local buffer into the connection message buffer
			connection.AppendPartialMessageToBuffer(message);

			//Only broadcast the message once it has fully arrived
			if (messageIsComplete)
			{
				message = userLabel + message;
				message = connection.GetMessageFromBuffer();

				Broadcast(message, connection.publicInfo.name, &connection);
				connection.ClearBuffer();
			}
		}
		else if (bytesRecv == 0)
		{
			ERR("Client disconnected");
			closesocket(connection.sock);
			m_clientConnections.erase(m_clientConnections.begin() + iClientConn);
			continue;
		}
		else if (bytesRecv == SOCKET_ERROR)
		{
			int error = WSAGetLastError();
			if (error != WSAEWOULDBLOCK)
			{
				ERR("Error receiving data from client, Err #" << WSAGetLastError());
				closesocket(connection.sock);
				m_clientConnections.erase(m_clientConnections.begin() + iClientConn);
				continue;
			}
		}
	}

}

//Filtered connection if nullptr means it is send from the server
void Server::Broadcast(std::string message, std::string from, Connection* pFilteredConnection)
{
	if (message.length() == 0)
	{
		ERR("Empty message");
		return;
	}
	if (message.length() > k_bufferSize)
	{
		ERR("Message too long");
		return;
	}
	//std::string finalMessage = message;
	std::string finalMessage = from + ": " + message;
	
	for (Connection& connection : m_clientConnections)
	{
		if (pFilteredConnection != nullptr && *pFilteredConnection == connection)
		{
			continue;
		}
		send(connection.sock, finalMessage.c_str(), int(finalMessage.size() + 1), 0);
	}
}

fd_set Server::MakeCopySet()
{
	fd_set copySet;
	FD_ZERO(&copySet);
	for (Connection& connection : m_clientConnections)
	{
		FD_SET(connection.sock, &copySet);
	}

	//Add listening socket
	FD_SET(m_serverSocket, &copySet);

	return copySet;
}

void Server::Run()
{
	Log("Server running");

	while (m_running)
	{
		CheckForClientConnections();

		CheckForClientMessages();

		//Sleep
		std::this_thread::sleep_for(std::chrono::milliseconds(k_serverPollingRate));
	}

	Log("Server shutting down");

	//Cleanup
	shutdown(m_serverSocket, SD_BOTH);
	closesocket(m_serverSocket);
}