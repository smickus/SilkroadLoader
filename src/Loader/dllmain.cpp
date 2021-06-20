// dllmain.cpp : Defines the entry point for the DLL application.
#include <WinSock2.h>
#include <stdio.h>
#include <windows.h>
#include "Detours.h"
#include <iostream>
#include <fstream>
#include <string>
using namespace std;

#pragma comment(lib, "Detours.lib")
#pragma comment(lib, "ws2_32.lib")

typedef int (WINAPI * trampoline_connect)(SOCKET s, const sockaddr *name, int namelen);
trampoline_connect orginal_connect;

struct RedirectionConfiguration {
	string originalAddress;
	USHORT originalPort;
	string proxyAddress;
	USHORT proxyPort;
};

RedirectionConfiguration ReadConfiguration() {
	RedirectionConfiguration configuration;
	configuration.originalAddress = getenv("ZBOT_ORIGINAL_ADDRESS");
	configuration.originalPort = (USHORT)strtoul(getenv("ZBOT_ORIGINAL_PORT"), NULL, 0);
	configuration.proxyAddress = getenv("ZBOT_PROXY_ADDRESS");
	configuration.proxyPort = (USHORT)strtoul(getenv("ZBOT_PROXY_PORT"), NULL, 0);
	return configuration;
}

int WINAPI my_connect(SOCKET s, const sockaddr *name, int namelen)
{
	sockaddr_in si;
	memcpy(&si, name, sizeof(sockaddr_in));

	RedirectionConfiguration configuration = ReadConfiguration();
	std::cout << "[Loader]Will be detouring connections from "
		<< configuration.originalAddress << ":" << configuration.originalPort
		<< " to "
		<< configuration.proxyAddress << ":" << configuration.proxyPort
		<< std::endl;

	std::cout << "[Loader]Connection detected: " << configuration.originalAddress << ":" << configuration.originalPort << std::endl;
	if (si.sin_addr.S_un.S_addr == inet_addr(configuration.originalAddress.c_str()) && si.sin_port == ntohs(configuration.originalPort)) {
		std::cout << "[Loader]Connection will be detoured to: " << configuration.proxyAddress << ":" << configuration.proxyPort << std::endl;
		si.sin_addr.S_un.S_addr = inet_addr(configuration.proxyAddress.c_str());
		si.sin_port = ntohs(configuration.proxyPort);
	}
	else {
		std::cout << "[Loader]Connection will not be detoured" << std::endl;
	}
	return orginal_connect(s, (sockaddr*)&si, sizeof(sockaddr_in));
}

bool isDebugMode()
{
	return strcmp(getenv("ZBOT_LOADER_DEBUG"), "true") == 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			if (isDebugMode()) {
				AllocConsole();
				freopen("CONOUT$", "w", stdout);
			}

			std::cout << "[Loader]Injecting connection detour routine" << std::endl;
			orginal_connect = (trampoline_connect)DetourFunction((PBYTE)GetProcAddress(GetModuleHandleA("ws2_32.dll"), "connect"), (PBYTE)my_connect);

			break;
	}
	return TRUE;
}
