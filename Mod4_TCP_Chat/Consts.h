#pragma once

constexpr int k_consoleColor_Red = 12;
constexpr int k_consoleColor_Green = 10;
constexpr int k_consoleColor_Yellow = 14;
constexpr int k_consoleColor_White = 15;
constexpr int k_consoleColor_Blue = 9;

constexpr int k_defaultPort = 8833;

constexpr int k_maxClientSendSize = 4;
constexpr int k_serverConnectionBufferSize = 256;

// Set this to true for final build
constexpr bool k_releaseMode = false;

constexpr bool k_useDefaultNames = !k_releaseMode;
constexpr bool k_useLocalHost = !k_releaseMode;
constexpr bool k_useDefaultPort = !k_releaseMode;

constexpr int k_serverPollingRate = 300;//milliseconds
constexpr int k_clientPollingRate = 300;//milliseconds

#define EXTRA_LOGGING 0