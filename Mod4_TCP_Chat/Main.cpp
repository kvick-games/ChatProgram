//TCP Chat Server and Client

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h> // inet_pton
#include <cstdio> //for freopen_s

#pragma comment(lib, "Ws2_32.lib")

#include "Globals.h"
#include "ClientBase.h"
#include "Server.h"

AppBase* g_pApp;
bool g_cleanupComplete = false;

BOOL WINAPI ConsoleHandler(DWORD signal)
{
	if (signal == CTRL_CLOSE_EVENT)
	{
		//LOG("Console window closing");
		if (g_pApp != nullptr)
		{
			g_pApp->Cleanup();
			g_pApp->Close();

			//while (!g_cleanupComplete)
			//{
			//	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			//}
		}

		return TRUE;
	}
	return FALSE;
}

std::string GetRandomUserName()
{
	std::string names[] = { "Chongo Wungus", "Bingy Singy", "Dingy Wingy", "Fingy Mingy", "Gingy Pingy", "Zing bang" };
	int nameIndex = rand() % (sizeof(names) / sizeof(names[0]));
	return names[nameIndex];
}

void CreateClientProcess(std::string consoleTitle)
{
	if (consoleTitle.empty())
	{
		ERR("Invalid console title");
		return;
	}

	std::string programName = "Mod4_TCP_Chat.exe";
	std::string cmdStr = programName + " client \"" + consoleTitle + "\"";

	//Spawn a new client process
	PROCESS_INFORMATION pi = {};
	STARTUPINFOA si;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	//Create the process
	if (!CreateProcessA(
		NULL,
		(LPSTR)cmdStr.c_str(),
		NULL,
		NULL,
		FALSE,
		CREATE_NEW_CONSOLE, //Create a new console window
		NULL,
		NULL,
		&si,
		&pi
	))
	{
		ERR("CreateProcess failed, Err #" << GetLastError());
	}
	else
	{
		LOG("Client process created");
	}

	//OLD CONSOLE CREATION CODE
	//std::string cmdStr = "cmd.exe /K echo Console Window: ";
	//cmdStr += consoleTitle;

	//Set the console title
	/*
	int lengthOfTitle = MultiByteToWideChar(CP_UTF8, 0, cmdStr.c_str(), -1, NULL, 0);
	wchar_t* pWideString = new wchar_t[lengthOfTitle];
	MultiByteToWideChar(CP_UTF8, 0, cmdStr.c_str(), -1, pWideString, lengthOfTitle);
	SetConsoleTitle(pWideString);

	STARTUPINFO si = { sizeof(si) };
	si.dwFlags = STARTF_USESTDHANDLES;

	if (CreateProcess(
		NULL,
		(LPWSTR)pWideString,
		NULL,
		NULL,
		FALSE,
		CREATE_NEW_CONSOLE,
		NULL,
		NULL,
		&si,
		&pi)
		) 
	{
		WaitForSingleObject(pi.hProcess, INFINITE);

		
	}
	else
	{
		ERR("CreateProcess failed, Err #" << GetLastError());
	}

	delete[] pWideString;
	*/
	/*
	AllocConsole();

	freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
	std::cout << "This is a new console window\n";
	*/

	std::this_thread::sleep_for(std::chrono::seconds());
}


int main(int argc, char* argv[])
{
	if (!SetConsoleCtrlHandler(ConsoleHandler, TRUE))
	{
		ERR("Could not set control handler");
		return -1;
	}

	//Seed random number generator
	srand(unsigned(time(NULL)));
	rand();


	//Convert command line arguments to strings
	std::vector<std::string> args;
	for (int i = 0; i < argc; i++)
	{
		args.push_back(argv[i]);
	}

//#define TESTING_CLIENT
//#define TESTING_SERVER

#ifdef TESTING_CLIENT
	{
		args.push_back("client");
		args.push_back("\"Chongo Wungus\"");
	}
#endif
#ifdef TESTING_SERVER
	{
		args.push_back("server");
	}
#endif


	//Initialize Winsock
	WSADATA wsData;
	WORD ver = MAKEWORD(2, 2);
	if (0 != WSAStartup(ver, &wsData))
	{
		ERR("Can't start Winsock, Err #" << WSAGetLastError());
		return -1;
	}

	bool runServer = false;
	bool runClient = false;

	//Check command line arguments
	if (args.size() > 1)
	{
		if (args[1] == "server")
		{
			runServer = true;
		}
		else if (args[1] == "client")
		{
			runClient = true;
			Client client;
			g_pApp = &client;

			std::string userName;

			if (k_useDefaultNames)
			{
				userName = GetRandomUserName();
			}
			else
			{
				userName = args[2];
			}

			client.SetupApp(userName, k_consoleColor_Green);

			client.Init();
			client.Run();

			client.Cleanup();
		}
	}
	
	//TEST CODE - RUNS SERVER AND CLIENTS
	if (!runServer && !runClient)
	{
		//Run test case where a server and two clients are spawned
		runServer = true;

		//Create client processes
		CreateClientProcess("\"Chongo Wungus\"");
		CreateClientProcess("\"Bingy Singy\"");
	}

	if (runServer)
	{
		//Run the server
		LOG("Starting up server");
		Server server;
		g_pApp = &server;

		server.Init();
		server.Run();
	}

	g_cleanupComplete = true;

	//Cleanup
	WSACleanup();
}