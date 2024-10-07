#include "ClientBase.h"

#include <Windows.h>
#define WIN32_LEAN_AND_MEAN

#include <WinSock2.h>
#include <WS2tcpip.h> // inet_pton
#pragma comment(lib, "Ws2_32.lib")

#include "Server.h"
#include "Globals.h"



Client::Client()
{

}

Client::~Client()
{
	
}

void Client::Cleanup()
{
	std::string msg = "exiting\n";
	send(m_sock, msg.c_str(), int(msg.size() + 1), 0);

	shutdown(m_sock, SD_BOTH);

	//Close the socket
	closesocket(m_sock);

	Log("Client cleanup");
}

int Client::Init()
{
	//Create a console window for the client
	AllocConsole();

	FILE* pFileDummy;
	freopen_s(&pFileDummy, "CONOUT$", "w", stdout);
	freopen_s(&pFileDummy, "CONOUT$", "w", stderr);
	std::cout.clear();
	std::cerr.clear();

	Print("Client console initialized");

	//Initialize Winsock
	m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == m_sock)
	{
		ERR("Can't create client socket, Err #" << WSAGetLastError());
		return -1;
	}

	if (!k_useDefaultNames)
	{
		m_name = "";
		while (m_name.empty())
		{
			std::string inputName;
			std::cout << "Enter your name: ";
			std::cin >> inputName;

			if (inputName.empty())
			{
				ERR("Invalid name");
			}
			else
			{
				m_name = inputName;
			}
		}
	}
	

	return 0;
}

void Thread_CheckForServerMessages(Client* pClient)
{
	bool m_threadRunning = true;
	fd_set readfds;
	struct timeval timeout;

	timeout.tv_sec = 1;
	timeout.tv_usec = 0;

	while (m_threadRunning)
	{
		if (pClient->GetClientState() == EClientState::Connected)
		{
			FD_ZERO(&readfds);
			FD_SET(pClient->m_sock, &readfds);

			int activity = select(0, &readfds, NULL, NULL, NULL);
			if (activity == SOCKET_ERROR)
			{
				pClient->Err("Select error, Err #" + std::to_string(WSAGetLastError()));
				break;
			}

			if (activity > 0 && FD_ISSET(pClient->m_sock, &readfds))
			{

				//Receive data from server
				int bytesRead = recv(pClient->m_sock, pClient->m_buffer_send, pClient->k_bufferSize, 0);

#if EXTRA_LOGGING == 1
				pClient->Log("Received: " + std::to_string(bytesRead) + "Bytes");
#endif

				if (bytesRead > 0)
				{
#if EXTRA_LOGGING == 1
					pClient->Log("Recv from Server: " + std::string(pClient->m_buffer));
#else
					pClient->Print(std::string(pClient->m_buffer_send), k_consoleColor_White);
#endif
				}
				else if (bytesRead == 0)
				{
					pClient->Log("Server disconnected");
					pClient->SetClientState(EClientState::Disconnected);
				}
				else
				{
					pClient->Err("Error receiving data, Err #" + std::to_string(WSAGetLastError()));
				}
			}

		}

		if (!pClient->m_running)
		{
			m_threadRunning = false;
		}
		
		std::this_thread::sleep_for(std::chrono::milliseconds(k_clientPollingRate));
	}
}

void Client::SetClientState(EClientState state)
{
	m_state = state;
}



void Client::Run()
{
	std::thread thr_serverCheck(Thread_CheckForServerMessages, this);

	while (m_running)
	{
		switch (m_state)
		{
		//===================================================================================
			case EClientState::Disconnected:
			{
				std::string inputPort;
				if (k_useLocalHost)
				{
					//m_serverIp = "127.0.0.1";
					m_serverIp = "localhost";
					m_serverPort = 8833;
				}
				else
				{
					//Query for server IP
					std::cout << "Enter server IP: ";
					std::cin >> m_serverIp;

					//TODO: Error check IP 
					m_serverPort = AskUserForPort();
				}

				//-------------------------------------------------
				//Resolve server IP
				sockaddr_in addr = {};
				addrinfo* pResult = nullptr;
				struct addrinfo hints = {};
				memset(&hints, 0, sizeof(hints));
				hints.ai_family = AF_INET; //IPv4
				hints.ai_socktype = SOCK_STREAM; //TCP

				int getaddrinfoResult = getaddrinfo(m_serverIp.c_str(), std::to_string(m_serverPort).c_str(), NULL, &pResult);
				if (getaddrinfoResult != 0)
				{
					ERR("getaddrinfo failed with error: " << getaddrinfoResult << ":" << gai_strerror(getaddrinfoResult));
					break;
				}

				Log("getaddrinfo result:" + std::to_string(getaddrinfoResult));
				if (pResult)
				{
					for (struct addrinfo* pInfo = pResult; pInfo != nullptr; pInfo = pInfo->ai_next)
					{
						void* pAddr;
						std::string ipVersion;

						if (pInfo->ai_family == AF_INET)
						{
							struct sockaddr_in* pIPv4 = (struct sockaddr_in*)pInfo->ai_addr;
							pAddr = &(pIPv4->sin_addr);
							ipVersion = "IPv4";

							addr = *pIPv4;
						}
						else
						{
							//TODO: IPv6, log error for now
							ERR("IPv6 not supported");
						}
					}

					//Copy to the addr
					//memcpy(&addr, pResult->ai_addr, pResult->ai_addrlen);
					
					freeaddrinfo(pResult);
				}
				//-------------------------------------------------
				
				addr.sin_family = AF_INET;
				addr.sin_port = htons(m_serverPort);
				inet_pton(addr.sin_family, m_serverIp.c_str(), &addr.sin_addr);

				//If timeout is > 0, set the timeout for the socket
				//Otherwise, don't set a timeout
				int timeout = -1;

				if (!k_useLocalHost)
				{
					timeout = 5000; //milliseconds
				}

				int sleepTime = 1000;//milliseconds
				int attempts = 0;

				//Poll for a connection
				bool success = true;
				std::string addrAsString = m_serverIp + std::string(":") + std::to_string(m_serverPort);
				Print("Attempting connection to server... " + addrAsString);
				while (SOCKET_ERROR == connect(m_sock, (sockaddr*)&addr, sizeof(addr)))
				{
					if (timeout == 0)
					{
						ERR("Can't connect to server, Err #" << WSAGetLastError());
						success = false;
						break;
					}
					ERR("No response from server... trying again: " << ++attempts);

					std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
					timeout -= sleepTime;
				}

				if (success)
				{
					Print("Connected to server!");
					SetClientState(EClientState::Connected);

					//Send initial client info
					ClientPublicInfo clientInfo;
					clientInfo.ip = m_serverIp;
					clientInfo.name = m_name;

					//TODO: Send whole struct
					//For now, just send the name
					send(m_sock, clientInfo.name.c_str(), sizeof(clientInfo.name), 0);
				}
				break;
				
			}
		//===================================================================================
			case EClientState::Connected:
			{
				//Poll for input from user
				std::string userInput;
				std::getline(std::cin, userInput);

				if (userInput == "exit" || userInput == "quit")
				{
					m_running = false;
					break;
				}

				//Delimit input with newline
				userInput += '\n';

				//Send data to server in packets
				char sendBuffer[k_maxClientSendSize];
				int start = 0;
				int remainingLength = static_cast<int>(userInput.length());

				while (remainingLength > 0)
				{
					int packetSendSize = remainingLength > k_maxClientSendSize ? k_maxClientSendSize : remainingLength;
					memcpy(sendBuffer, userInput.c_str() + start, packetSendSize);
					send(m_sock, sendBuffer, packetSendSize, 0);

					start += packetSendSize;
					remainingLength -= packetSendSize;
				}
				
				break;
			}
		}
	}

	Cleanup();
	thr_serverCheck.join();

	
}