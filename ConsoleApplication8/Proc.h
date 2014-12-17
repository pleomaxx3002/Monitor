#pragma once
#include <Windows.h>
#include "Logger.h"




class Proc
{
private:
	friend class Monitor;
	LPTSTR path;												//Path to the exe file
	LPTSTR lpCommandLine;										//Command line attributes
	SECURITY_ATTRIBUTES ProccessAttributes, ThreadAttributes;	//SecurityAttributes for Process & Thread
	DWORD CreationFlags;										//Creation Flags for Process
	LPVOID lpEnvironment;										//Pointer to process environment
	LPTSTR lpCurrDirr;											//Pointer to process dirrectory path
	LPSTARTUPINFO s;											//Startupinfo for process
	Logger *pLog;												//Logger obj to Log
	PROCESS_INFORMATION prInf;									//Process information structure
	Proc(DWORD, Logger *, bool &);								//Connecting to existing process
	Proc(LPCTSTR, Logger *, bool &, LPTSTR CommLine = NULL, LPSECURITY_ATTRIBUTES ProcSecAtt = NULL, LPSECURITY_ATTRIBUTES ThrSecAtt = NULL, DWORD CrFlags = 0, LPVOID lpEnv = NULL, LPCTSTR lpCurDir = NULL, LPSTARTUPINFO StINF = NULL);
																//Creating new process
	bool Start();												//True if started succes, false else
	bool Stop();												//True if started succes, false else
	bool Restart();												//True if started succes, false else
	HANDLE hWait;												//Handle for restart function
	HANDLE Access;												//Mutex to prevent double access to process
	DWORD EXITCode;												//Exit code
	~Proc();
	Proc(const Proc&);											//Preventing Copying
	Proc& operator=(const Proc&);								//Preventing assigning
	static void CALLBACK RestartCallback(void*, BOOLEAN);		//Callback function for restart
	static void CALLBACK StopCallback(void*, BOOLEAN);			//Callback function to stop
	int Status;													//Current process status (1 - still runing, 2 - stopped, -1 - Creating error, 0 - restarting) 
};

