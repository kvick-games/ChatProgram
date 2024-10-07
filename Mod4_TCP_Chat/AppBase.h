#pragma once

#include <string>
#include "Consts.h"

class AppBase
{
public:
	void SetupApp(std::string name, int color);
	void Close();
	virtual	void Cleanup();

	void Log(std::string message);

	//Prints user-facing message as opposed to debug
	void Print(std::string message, int customColor = -1);
	//Log printf style
	template<typename... Args>
	void Log(const char* format, Args... args)
	{
		char buffer[256];
		sprintf_s(buffer, format, args...);
		
		printf("%s: %s\n", m_name.c_str(), buffer);
	}
	void Err(std::string message);

	int AskUserForPort();


public:
	int m_serverPort;

	bool m_running = true;

	std::string m_name;
	int m_color = k_consoleColor_Green;//Windows console color, green by default
};