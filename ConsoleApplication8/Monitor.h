#pragma once
#include <Windows.h>
#include <vector>
#include "Proc.h"
#include "Logger.h"

class Monitor
{
private:
	std::vector <Proc * > P;				//vector of Process(pointers in case of relocation)
	HANDLE vectMutex, termMutex;			//mutexes to prevent double access to vector
	Logger *pLog;							//logger pointer


public:
	Monitor(Logger *);
	bool CreateProc(LPCTSTR, LPTSTR CommLine = NULL, LPSECURITY_ATTRIBUTES ProcSecAtt = NULL, LPSECURITY_ATTRIBUTES ThrSecAtt = NULL, DWORD CrFlags = 0, LPVOID lpEnv = NULL, LPCTSTR lpCurDir = NULL, LPSTARTUPINFO StINF = NULL);
											//Pushing new Process
	bool TerminateProc(int);				//Terminating i-th process
	const HANDLE* GetHandle(int);			//return const pointer to handle of i-th process
	const DWORD GetID(int);					//return Id of i-th process
	const int GetStatus(int);				//return status of i-th process
	~Monitor();
	int EnumProc(DWORD*);					//return the array of id-s of running processes
	bool ConnectToProcess(DWORD);			//connecting to process with input Id
	static void RestartCallback(Monitor*, int);//Callback for restarting i-th process
	static void CreateCallback(Monitor*, LPTSTR);//Callback for creating process
	static void StopCallback(Monitor*, int);//Callback for stopping i-th process
};

