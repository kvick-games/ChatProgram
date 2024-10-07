#pragma once

#include <iostream>
#include <string>

#include <WinSock2.h>
#include <WS2tcpip.h> // inet_pton

#pragma comment(lib, "Ws2_32.lib")

#include <thread>

#include "Consts.h"

#define LOG(x) std::cout << x << std::endl
#define LOG_SERVER(x) std::cout << "SERVER: " << x << std::endl
#define LOG_(x) std::cout << x << std::endl
#define ERR(x) std::cerr << x << std::endl

int Min(int a, int b);

struct ClientPublicInfo
{
	std::string name;
	std::string ip;
	std::string port;

	std::string ConstructUserDisplayName()
	{
		return name + " [" + ip + ":" + port + "]";
	}
};

struct Connection
{
	Connection()
	{
		ClearBuffer();
	}

	bool operator==(const Connection& other) const
	{
		return (sock == other.sock);
	}

	void ClearBuffer()
	{
		memset(buffer, 0, k_serverConnectionBufferSize);
		currentBufferEnd = 0;
	}

	void AppendPartialMessageToBuffer(std::string partialMessage)
	{
		int bytesReceived = partialMessage.length();
		memcpy(buffer + currentBufferEnd, partialMessage.c_str(), bytesReceived);
		currentBufferEnd += bytesReceived;
	}

	std::string GetMessageFromBuffer()
	{
		std::string message = buffer;

		//Get the message up to the first newline
		size_t newlinePos = message.find('\n');
		if (newlinePos != std::string::npos)
		{
			message = message.substr(0, newlinePos);
		}
		else
		{
			message = "ERROR: NO NEWLINE DELIMITER FOUND IN MESSAGE STRING";
		}

		return message;
	}

	char buffer[k_serverConnectionBufferSize];
	int currentBufferEnd = 0;
	SOCKET sock;
	sockaddr_in addr;
	ClientPublicInfo publicInfo;
};