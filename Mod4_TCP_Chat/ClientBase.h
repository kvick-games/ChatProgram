#pragma once

#include "AppBase.h"

#include "Socket.h"

#include "Globals.h"

class Server;

enum class EClientState
{
	Disconnected,
	Connecting,
	Connected
};

class Client : public AppBase
{
	public:
	Client();
	~Client();

	int Init();

	void Run();

	virtual void Cleanup() override;

	

	//TODO: Needs to be thread safe
	EClientState GetClientState() { return m_state; }
	void SetClientState(EClientState state);

public:
	//Server* m_pServer;

	EClientState m_state = EClientState::Disconnected;

	bool m_connected = false;

	SOCKET m_sock;

	constexpr static int k_bufferSize = 256;
	char m_buffer_send[k_bufferSize];

	std::string m_serverIp;
	
};