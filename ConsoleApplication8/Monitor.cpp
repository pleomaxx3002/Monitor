#include "stdafx.h"
#include "Monitor.h"
#include "Proc.h"
#include <Windows.h>
#include <Psapi.h>


Monitor::Monitor(Logger *Log)
{
	pLog = Log;
	vectMutex = CreateMutex(NULL, FALSE, NULL);
	termMutex = CreateMutex(NULL, FALSE, NULL);
}


Monitor::~Monitor()
{
	WaitForSingleObject(vectMutex, 10000);
	for (int i = 0, n = P.size(); i < n; i++)
		delete P[i];
	CloseHandle(vectMutex);
	CloseHandle(termMutex);
}


bool Monitor::CreateProc(LPCTSTR path,
	LPTSTR CommLine,
	LPSECURITY_ATTRIBUTES ProcSecAtt,
	LPSECURITY_ATTRIBUTES ThrSecAtt,
	DWORD CrFlags,
	LPVOID lpEnv,
	LPCTSTR lpCurDir,
	LPSTARTUPINFO StINF)
{
	bool res;
	if(WaitForSingleObject(vectMutex, 1000)==ERROR_TIMEOUT)
		return false;
	P.push_back(new Proc(path, pLog, res,CommLine,ProcSecAtt,ThrSecAtt,CrFlags,lpEnv,lpCurDir,StINF));
	if (!res)
	{
		delete P[P.size() - 1];
		P.pop_back();
	}
	ReleaseMutex(vectMutex);

	return true;
}

bool Monitor::TerminateProc(int i)
{
	if (WaitForSingleObject(vectMutex, 1000) == ERROR_TIMEOUT)
	{
		
		ReleaseMutex(vectMutex);
		return false;
	}
	Proc *pk = P[i];
	ReleaseMutex(vectMutex);
	WaitForSingleObject(pk->Access, INFINITY);
	/*DWORD P1;
	if (GetExitCodeProcess(pk->prInf.hProcess, &P1) == FALSE)
	{
		ReleaseMutex(pk->Access);
		return false;
	}
	if (P1 != STILL_ACTIVE)
		return false;*/
	pk->Stop();
	ReleaseMutex(pk->Access);
	if (WaitForSingleObject(pk->prInf.hProcess, 3000) == WAIT_TIMEOUT)
		return false;
	return true;
}


const HANDLE* Monitor::GetHandle(int i)
{
	WaitForSingleObject(vectMutex,INFINITE);
	HANDLE* res;
	if (i >= P.size())
		res = NULL;
	else
		res = &(P[i]->prInf.hProcess);
	ReleaseMutex(vectMutex);
	return res;
}

const DWORD Monitor::GetID(int i)
{
	WaitForSingleObject(vectMutex, INFINITE);
	DWORD res;
	if (i >= P.size())
		res = -1;
	else
		res = P[i]->prInf.dwProcessId;
	ReleaseMutex(vectMutex);
	return res;
}

const int Monitor::GetStatus(int i)
{ 
	WaitForSingleObject(vectMutex, INFINITE);
	int res;
	if (i >= P.size())
		res = -1;
	else
		res = P[i]->Status;
	ReleaseMutex(vectMutex);
	return res;
}


void Monitor::RestartCallback(Monitor *This, int I)
{
	WaitForSingleObject(This->vectMutex, INFINITE);
	if (I < This->P.size())
		This->P[I]->Restart();
	ReleaseMutex(This->vectMutex);
}

void Monitor::StopCallback(Monitor *This, int I)
{
	WaitForSingleObject(This->vectMutex, INFINITE);
	if (This->P.size()>I)
		This->P[I]->Stop();
	ReleaseMutex(This->vectMutex);
}

void Monitor::CreateCallback(Monitor *This, LPTSTR path)
{
	This->CreateProc(path);
}

int Monitor::EnumProc(DWORD *pArr)
{
	DWORD i = 0;
	if (!EnumProcesses(pArr, sizeof(pArr), &i))
		return -1;
	return i / sizeof(DWORD);
}

bool Monitor::ConnectToProcess(DWORD ID)
{
	WaitForSingleObject(vectMutex, INFINITE);
	bool res;
	P.push_back(new Proc(ID,pLog,res));
	if (!res)
	{
		delete P[P.size() - 1];
		P.pop_back();
	}
	ReleaseMutex(vectMutex);
	return res;
}