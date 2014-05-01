/*
 - The WP81ProjectionClient Project -
	Author:ShanYe [K.F Yang] (in 2014-04-24)
	Mail:chinatb361@hotmail.com
	Target Platform:Windows 7 or latter and WinUsb.sys
	Develop Environment:Visual Studio 2013 with Visual C++
	
	This Project Implement by Reverse Engineering. (Project My Screen App - 8.10.12349)
*/

#include <Windows.h>

BOOL APIENTRY DllMain(HMODULE hModule,DWORD  ul_reason_for_call,LPVOID lpReserved)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hModule);
	return TRUE;
}