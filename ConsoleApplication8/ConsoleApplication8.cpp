// ConsoleApplication8.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
#include "Proc.h"
#include "Monitor.h"
#include <iostream>
#include <fstream>
#include <functional>



std::wofstream c("Log.txt");

MyLogger A(c);


Monitor a(&A);
int i = 0;

DWORD WINAPI ThreadFunction(LPVOID)
{
	a.CreateProc(TEXT("C:\\Users\\Admin\\Desktop\\notepad_AkelUndo.exe"));
	Sleep((rand()%10000)*100);
	a.TerminateProc(i++);
	return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
	/*HANDLE ThreadsArray[15];
	DWORD ThreadID;
	for (int i = 0; i < 15; i++)
		ThreadsArray[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadFunction, NULL, 0, &ThreadID);
	
	WaitForMultipleObjects(15, ThreadsArray, TRUE, INFINITY);

	for (int i = 0; i < 15; i++)
		CloseHandle(ThreadsArray[i]);*/
	a.CreateProc(TEXT("C:\\Users\\Admin\\Desktop\\notepad_AkelUndo.exe"));
	//Sleep(10000);
	//std::function<void(LPTSTR)> F = std::bind(&Monitor::CreateCallback, &a, std::placeholders::_1);
	//a.ConnectToProcess(9084);
	//a.TerminateProc(0);
	Sleep(5000);
	a.TerminateProc(0);
	//a.ConnectToProcess(10208);
	system("pause");


	return 0;
}
