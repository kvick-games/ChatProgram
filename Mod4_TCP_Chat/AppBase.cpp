#include "AppBase.h"

#define WIN32_LEAN_AND_MEAN

#include <stdio.h>
#include <iostream>
#include <Windows.h>

#include "Consts.h"

void AppBase::SetupApp(std::string name, int color)
{
	m_name = name; 
	m_color = color;
}

void AppBase::Close()
{
	m_running = false;
}

void AppBase::Cleanup()
{
	//
}

void AppBase::Log(std::string message)
{
	//Set console text color to green
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), m_color);

	std::cout << m_name << ": " << message << std::endl;
}

void AppBase::Print(std::string message, int customColor)
{
	int printColor = (customColor >= 0) ? customColor : m_color;
	//Set console text color to green
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), printColor);

	std::cout << message << std::endl;
}

void AppBase::Err(std::string message)
{
	//Set console text color to red
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);

	std::cerr << m_name << ": " << message << std::endl;
}

int AppBase::AskUserForPort()
{
	if (k_useDefaultPort)
	{
		return k_defaultPort;
	}

	std::string inputPort;
	bool validPort = false;
	while (!validPort)
	{
		//Query for Port
		std::cout << "Enter port (range 0 - 65535): ";
		std::cin >> inputPort;

		//Check if input is a number
		validPort = (inputPort.find_first_not_of("0123456789") == std::string::npos);
	}

	//Convert input port to int
	int port = std::stoi(inputPort);
	return port;
}