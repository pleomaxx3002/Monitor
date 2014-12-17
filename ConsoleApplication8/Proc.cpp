#include "stdafx.h"
#include "Proc.h"
#include <Psapi.h>

#ifdef UNICODE
#define LoadLibraryFunc "LoadLibraryW"
#else
#define LoadLibraryFunc "LoadLibraryA"
#endif


#define MAX_PATH_SIZE 200			//Max size of path string



Proc::Proc(LPCTSTR inp_path,
	Logger *inLog,
	bool &res,
	LPTSTR CommLine,
	LPSECURITY_ATTRIBUTES ProcSecAtt,
	LPSECURITY_ATTRIBUTES ThrSecAtt, 
	DWORD CrFlags, 
	LPVOID lpEnv, 
	LPCTSTR lpCurDir, 
	LPSTARTUPINFO StINF)
{
	Access = CreateMutex(NULL, FALSE, NULL);
	WaitForSingleObject(Access, INFINITE);
	free(path);
	path = _tcsdup(inp_path);
	s = StINF;
	pLog = inLog;
	lpCommandLine = _tcsdup(CommLine);
	ProccessAttributes = ProcSecAtt == NULL ? SECURITY_ATTRIBUTES() : *ProcSecAtt; 
	ThreadAttributes = ThrSecAtt == NULL ? SECURITY_ATTRIBUTES() : *ThrSecAtt;
	CreationFlags=CrFlags;
	lpEnvironment = lpEnv;
	lpCurrDirr = _tcsdup(lpCurDir);

	if (StINF == NULL)
	{
		s = new STARTUPINFO();
		s->cb = sizeof(STARTUPINFO);
		s->lpReserved = NULL;
		s->lpDesktop = NULL;
		s->lpTitle = NULL;
		s->dwFlags = STARTF_USESHOWWINDOW;
		s->cbReserved2 = 0;
		s->lpReserved2 = 0;
		s->wShowWindow = SW_SHOWDEFAULT;
	}
	else
		s = new STARTUPINFO(*StINF);
	res = Start();
	if (res == true)
		Status = STILLRUNNING;
	else
	{
		Status = CREATINGERROR;
		(*pLog)(LOG_CRITICAL, TEXT("Creating \"%S\" process failue."), path);
		ReleaseMutex(Access);
		return;
	}
	EXITCode = STILL_ACTIVE;
	RegisterWaitForSingleObject(&hWait, prInf.hProcess, RestartCallback, this, INFINITE, WT_EXECUTEONLYONCE);
	ReleaseMutex(Access);
}


Proc::~Proc()
{
	Stop();
	if (lpEnvironment != NULL) free(lpEnvironment);
	if (lpCurrDirr != NULL) free(lpCurrDirr);
	if (s != NULL) free(s);
	if (lpCommandLine != NULL) free(lpCommandLine);
	if (path != NULL) free(path);
	if (hWait != NULL)
	{
		TerminateThread(hWait, NO_ERROR);
		WaitForSingleObject(hWait, 1000);
		//CloseHandle(hWait);
	}
	//CloseHandle(Access);
}


bool Proc::Stop()
{
	if (WaitForSingleObject(Access, 1000) == WAIT_TIMEOUT)
		return false;
	Status = STOPPED;
	if (TerminateProcess(prInf.hProcess, 0) == FALSE)
	{
		(*pLog)(LOG_CRITICAL, TEXT("Stopping %d ID Failue, terminate false"), prInf.dwProcessId);
		ReleaseMutex(Access);
		return false;
	}
	if (WaitForSingleObject(prInf.hProcess, 10000) == WAIT_TIMEOUT)
	{
		(*pLog)(LOG_CRITICAL, TEXT("Stopping %d ID Failue"), prInf.dwProcessId);
		ReleaseMutex(Access);
		return false;
	}
	GetExitCodeProcess(prInf.hProcess, &EXITCode);
	(*pLog)(LOG_USUALL, TEXT("Process \"%S\" manually stopped. ExitCode: %d."), path, EXITCode);
	ReleaseMutex(Access);
	return true;
}


bool Proc::Start()
{
	if (!NeedCurrentDirectoryForExePath(path))
		return false;
	if (!CreateProcess(path, lpCommandLine, &ProccessAttributes, &ThreadAttributes, FALSE, CreationFlags, lpEnvironment, lpCurrDirr, s, &prInf))
		return false;
	(*pLog)(LOG_USUALL, TEXT("Process \"%S\" created with %d ID"), path, prInf.dwProcessId);
	return true;
}

bool Proc::Restart()
{
	Status = RESTARTING;
	if (WaitForSingleObject(Access, 10000) == WAIT_TIMEOUT)
		return false;
	GetExitCodeProcess(prInf.hProcess, &EXITCode);
	if (EXITCode == STILL_ACTIVE)
	{
		(*pLog)(LOG_CRITICAL, TEXT("Restarting \"%S\" Failue. Proccess Still active"), path);
		Status = STILLRUNNING;
		RegisterWaitForSingleObject(&hWait, prInf.hProcess, RestartCallback, this, INFINITE, WT_EXECUTEONLYONCE);
		ReleaseMutex(Access);
		return false;
	}
	if (!Start())
	{
		(*pLog)(LOG_CRITICAL, TEXT("Restarting \"%S\" Failue"), path);
		Status = CREATINGERROR;
		RegisterWaitForSingleObject(&hWait, prInf.hProcess, RestartCallback, this, INFINITE, WT_EXECUTEONLYONCE);
		ReleaseMutex(Access);
		return false;
	}
	(*pLog)(LOG_CRITICAL, TEXT("Process \"%S\" restarted"), path);
	Status = STILLRUNNING;
	RegisterWaitForSingleObject(&hWait, prInf.hProcess, RestartCallback, this, INFINITE, WT_EXECUTEONLYONCE);
	ReleaseMutex(Access);
	return true;
}

Proc::Proc(const Proc &P)
{
}

void CALLBACK Proc::RestartCallback(void *p, BOOLEAN s)
{
	if (((Proc *)p)->Status == STOPPED)
		return;
	GetExitCodeProcess(((Proc *)p)->prInf.hProcess, &(((Proc *)p)->EXITCode));
	(*((Proc *)p)->pLog)(LOG_CRITICAL, TEXT("Process \"%S\" ended with exitcode %d."), ((Proc *)p)->path, ((Proc *)p)->EXITCode);
	((Proc *)p)->Restart();
}

void CALLBACK Proc::StopCallback(void *p, BOOLEAN s)
{
	((Proc *)p)->Stop();
}

Proc::Proc(DWORD ID, Logger* Log, bool &res)
{
	Access = CreateMutex(NULL, TRUE, NULL);
	prInf.hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ID);
	prInf.dwProcessId = ID;
	pLog = Log;
	if (prInf.hProcess == NULL)
	{
		(*pLog)(LOG_CRITICAL, TEXT("Connection to %d ID Failue"), ID);
		res = false;
		ReleaseMutex(Access);
		return;
	}
	path = new TCHAR[MAX_PATH_SIZE];
	if (GetModuleFileNameEx(prInf.hProcess, NULL, path, MAX_PATH_SIZE) == 0)
	{
		(*pLog)(LOG_CRITICAL, TEXT("Connection to %d ID Failue: Can't Get exe Path"), ID);
		CloseHandle(prInf.hProcess);
		res = false;
		ReleaseMutex(Access);
		return;
	}
	ProccessAttributes = ThreadAttributes = SECURITY_ATTRIBUTES();
	CreationFlags = 0;
	lpCurrDirr = NULL;
	HANDLE Temp;
	DWORD Id;

	//Getting lib location
	PVOID pLib;
	TCHAR szLibPath[MAX_PATH_SIZE];
	GetModuleFileNameEx(GetCurrentProcess(), NULL, szLibPath, MAX_PATH_SIZE);
	int i;
	for (i = _tcsclen(szLibPath); i > 0 && szLibPath[i - 1] != _T('\\'); --i);
	szLibPath[i++] = _T('l'); szLibPath[i++] = _T('i'); szLibPath[i++] = _T('b'); szLibPath[i++] = _T('.'); szLibPath[i++] = _T('d'); szLibPath[i++] = _T('l'); szLibPath[i++] = _T('l'); szLibPath[i] = _T('\0');
	HMODULE hKernel32 = GetModuleHandle(_T("Kernel32"));
	HMODULE libModule = GetModuleHandle(szLibPath);
	if (libModule == NULL)
	{
		(*pLog)(LOG_CRITICAL, TEXT("Connection to %d ID Failue: Can't get path to DLL"), ID);
		CloseHandle(prInf.hProcess);
		res = false;
		ReleaseMutex(Access);
		return;
	}


	pLib = VirtualAllocEx(prInf.hProcess, NULL, MAX_PATH_SIZE*sizeof(TCHAR), MEM_COMMIT, PAGE_READWRITE);
	if (pLib == NULL)
	{
		(*pLog)(LOG_CRITICAL, TEXT("Connection to %d ID Failue: Can't Allocatte memory in proc"), ID);
		CloseHandle(prInf.hProcess);
		res = false;
		ReleaseMutex(Access);
		return;
	}
	if (!WriteProcessMemory(prInf.hProcess, (LPVOID)pLib, szLibPath, MAX_PATH_SIZE*sizeof(TCHAR), NULL))
	{
		(*pLog)(LOG_CRITICAL, TEXT("Connection to %d ID Failue: Can't write memory in proc"), ID);
		VirtualFreeEx(prInf.hProcess, pLib, MAX_PATH_SIZE*sizeof(TCHAR), MEM_RELEASE);
		CloseHandle(prInf.hProcess);
		res = false;
		ReleaseMutex(Access);
		return;
	}


	Temp = CreateRemoteThread(prInf.hProcess, NULL, 0,
		(LPTHREAD_START_ROUTINE)GetProcAddress(hKernel32, LoadLibraryFunc),
		pLib, 0, NULL);
	if (Temp == NULL)
	{
		(*pLog)(LOG_CRITICAL, TEXT("Connection to %d ID Failue: Can't load dll in proc"), ID);
		VirtualFreeEx(prInf.hProcess, pLib, MAX_PATH_SIZE*sizeof(TCHAR), MEM_RELEASE);
		CloseHandle(prInf.hProcess);
		res = false;
		ReleaseMutex(Access);
		return;
	}
	if (WaitForSingleObject(Temp, 10000) == WAIT_TIMEOUT)
	{
		TerminateThread(Temp, NO_ERROR);
		(*pLog)(LOG_CRITICAL, TEXT("Connection to %d ID Failue: Can't load dll in proc"), ID);
		VirtualFreeEx(prInf.hProcess, pLib, MAX_PATH_SIZE*sizeof(TCHAR), MEM_RELEASE);
		CloseHandle(prInf.hProcess);
		res = false;
		ReleaseMutex(Access);
		return;

	}
	

	//getting handle of my dll
	
	DWORD loadedDLL = 0;
	GetExitCodeThread(Temp, &loadedDLL);
	CloseHandle(Temp);

	VirtualFreeEx(prInf.hProcess, pLib, MAX_PATH_SIZE*sizeof(TCHAR), MEM_RELEASE);


	//Getting STARTUPINFO
	pLib = VirtualAllocEx(prInf.hProcess, NULL, sizeof(STARTUPINFO), MEM_COMMIT, PAGE_READWRITE);
	if (pLib == NULL)
	{
		(*pLog)(LOG_CRITICAL, TEXT("Connection to %d ID Failue: Can't allocate memory for STARTUPINFO structure in proc"), ID);
		Temp = CreateRemoteThread(prInf.hProcess, NULL, 0,
			(LPTHREAD_START_ROUTINE) ::GetProcAddress(hKernel32,
			"FreeLibrary"),
			(void*)loadedDLL, 0, NULL);
		WaitForSingleObject(Temp, INFINITE);
		CloseHandle(Temp);
		VirtualFreeEx(prInf.hProcess, pLib, sizeof(STARTUPINFO), MEM_RELEASE);
		CloseHandle(prInf.hProcess);
		res = false;
		ReleaseMutex(Access);
		return;

	}
	Temp = CreateRemoteThread(prInf.hProcess, NULL, 0,
		(LPTHREAD_START_ROUTINE)GetProcAddress(libModule, "GetStIn"),
		pLib, 0, NULL);
	if (Temp == NULL)
	{ 
		(*pLog)(LOG_CRITICAL, TEXT("Connection to %d ID Failue: Can't get sturtupinfo of proc"), ID);
		Temp = CreateRemoteThread(prInf.hProcess, NULL, 0,
			(LPTHREAD_START_ROUTINE) ::GetProcAddress(hKernel32,
			"FreeLibrary"),
			(void*)loadedDLL, 0, NULL);
		WaitForSingleObject(Temp, 1000);
		CloseHandle(Temp);
		VirtualFreeEx(prInf.hProcess, pLib, sizeof(STARTUPINFO), MEM_RELEASE);
		CloseHandle(prInf.hProcess);
		res = false;
		ReleaseMutex(Access);
		return;
	}
	if (WaitForSingleObject(Temp, 10000)==WAIT_TIMEOUT)
	{ 
		(*pLog)(LOG_CRITICAL, TEXT("Connection to %d ID Failue: Can't get sturtupinfo of proc"), ID);
		TerminateProcess(Temp, NO_ERROR);
		WaitForSingleObject(Temp, 1000);
		CloseHandle(Temp);
		Temp = CreateRemoteThread(prInf.hProcess, NULL, 0,
			(LPTHREAD_START_ROUTINE) ::GetProcAddress(hKernel32,
			"FreeLibrary"),
			(void*)loadedDLL, 0, NULL);
		WaitForSingleObject(Temp, INFINITE);
		CloseHandle(Temp);
		VirtualFreeEx(prInf.hProcess, pLib, sizeof(STARTUPINFO), MEM_RELEASE);
		CloseHandle(prInf.hProcess);
		res = false;
		ReleaseMutex(Access);
		return;
	}
	if (!ReadProcessMemory(prInf.hProcess, pLib, s, sizeof(STARTUPINFO), NULL))
	{
		(*pLog)(LOG_CRITICAL, TEXT("Connection to %d ID Failue: Can't get sturtupinfo of proc"), ID);
		Temp = CreateRemoteThread(prInf.hProcess, NULL, 0,
			(LPTHREAD_START_ROUTINE) ::GetProcAddress(hKernel32,
			"FreeLibrary"),
			(void*)loadedDLL, 0, NULL);
		WaitForSingleObject(Temp, INFINITE);
		CloseHandle(Temp);
		VirtualFreeEx(prInf.hProcess, pLib, sizeof(STARTUPINFO), MEM_RELEASE);
		CloseHandle(prInf.hProcess);
		res = false;
		ReleaseMutex(Access);
		return;
	}
	VirtualFreeEx(prInf.hProcess, pLib, sizeof(STARTUPINFO), MEM_RELEASE);
	CloseHandle(Temp);


	//Getting CmdLine
	lpCommandLine = new TCHAR[MAX_PATH_SIZE];
	pLib = VirtualAllocEx(prInf.hProcess, NULL, MAX_PATH_SIZE*sizeof(TCHAR), MEM_COMMIT, PAGE_READWRITE);
	if (pLib == NULL)
	{
		(*pLog)(LOG_CRITICAL, TEXT("Connection to %d ID Failue: Can't allocate memory in proc"), ID);
		Temp = CreateRemoteThread(prInf.hProcess, NULL, 0,
			(LPTHREAD_START_ROUTINE) ::GetProcAddress(hKernel32,
			"FreeLibrary"),
			(void*)loadedDLL, 0, NULL);
		WaitForSingleObject(Temp, INFINITE);
		CloseHandle(Temp);
		CloseHandle(prInf.hProcess);
		res = false;
		ReleaseMutex(Access);
		return;
	}
	Temp = CreateRemoteThread(prInf.hProcess, NULL, 0,
		(LPTHREAD_START_ROUTINE)GetProcAddress(libModule, "GetCmdLine"),
		pLib, 0, NULL);
	if (Temp == NULL)
	{
		(*pLog)(LOG_CRITICAL, TEXT("Connection to %d ID Failue: Can't get cmd line of proc"), ID);
		VirtualFreeEx(prInf.hProcess, pLib, MAX_PATH_SIZE*sizeof(TCHAR), MEM_RELEASE);
		Temp = CreateRemoteThread(prInf.hProcess, NULL, 0,
			(LPTHREAD_START_ROUTINE) ::GetProcAddress(hKernel32,
			"FreeLibrary"),
			(void*)loadedDLL, 0, NULL);
		WaitForSingleObject(Temp, INFINITE);
		CloseHandle(Temp);
		CloseHandle(prInf.hProcess);
		res = false;
		ReleaseMutex(Access);
		return;
	}
	if (WaitForSingleObject(Temp, 10000) == WAIT_TIMEOUT)
	{
		(*pLog)(LOG_CRITICAL, TEXT("Connection to %d ID Failue: Can't get cmd line of proc"), ID);
		TerminateThread(Temp, NO_ERROR);
		WaitForSingleObject(Temp, 1000);
		CloseHandle(Temp);
		VirtualFreeEx(prInf.hProcess, pLib, MAX_PATH_SIZE*sizeof(TCHAR), MEM_RELEASE);
		Temp = CreateRemoteThread(prInf.hProcess, NULL, 0,
			(LPTHREAD_START_ROUTINE) ::GetProcAddress(hKernel32,
			"FreeLibrary"),
			(void*)loadedDLL, 0, NULL);
		WaitForSingleObject(Temp, INFINITE);
		CloseHandle(Temp);
		CloseHandle(prInf.hProcess);
		res = false;
		ReleaseMutex(Access);
		return;
	}
	if (!ReadProcessMemory(prInf.hProcess, pLib, lpCommandLine, MAX_PATH_SIZE*sizeof(TCHAR), NULL))
	{
		(*pLog)(LOG_CRITICAL, TEXT("Connection to %d ID Failue: Can't get cmd line of proc"), ID);
		VirtualFreeEx(prInf.hProcess, pLib, MAX_PATH_SIZE*sizeof(TCHAR), MEM_RELEASE);
		Temp = CreateRemoteThread(prInf.hProcess, NULL, 0,
			(LPTHREAD_START_ROUTINE) ::GetProcAddress(hKernel32,
			"FreeLibrary"),
			(void*)loadedDLL, 0, NULL);
		WaitForSingleObject(Temp, INFINITE);
		CloseHandle(Temp);
		CloseHandle(prInf.hProcess);
		res = false;
		ReleaseMutex(Access);
		return;
	}
	VirtualFreeEx(prInf.hProcess, pLib, MAX_PATH_SIZE*sizeof(TCHAR), MEM_RELEASE);
	CloseHandle(Temp);


	//Getting CurrDir

	lpCurrDirr= new TCHAR[MAX_PATH_SIZE];
	pLib = VirtualAllocEx(prInf.hProcess, NULL, MAX_PATH_SIZE*sizeof(TCHAR), MEM_COMMIT, PAGE_READWRITE);
	if (pLib == NULL)
	{
		(*pLog)(LOG_CRITICAL, TEXT("Connection to %d ID Failue: Can't allocate memory in proc"), ID);
		VirtualFreeEx(prInf.hProcess, pLib, MAX_PATH_SIZE*sizeof(TCHAR), MEM_RELEASE);
		Temp = CreateRemoteThread(prInf.hProcess, NULL, 0,
			(LPTHREAD_START_ROUTINE) ::GetProcAddress(hKernel32,
			"FreeLibrary"),
			(void*)loadedDLL, 0, NULL);
		WaitForSingleObject(Temp, INFINITE);
		CloseHandle(Temp);
		CloseHandle(prInf.hProcess);
		res = false;
		ReleaseMutex(Access);
		return;
	}
	Temp = CreateRemoteThread(prInf.hProcess, NULL, 0,
		(LPTHREAD_START_ROUTINE)GetProcAddress(libModule, "GetCurrDirr"),
		pLib, 0, NULL);
	if (Temp == NULL)
	{
		(*pLog)(LOG_CRITICAL, TEXT("Connection to %d ID Failue: Can't get current dirrectory of proc"), ID);
		VirtualFreeEx(prInf.hProcess, pLib, MAX_PATH_SIZE*sizeof(TCHAR), MEM_RELEASE);
		Temp = CreateRemoteThread(prInf.hProcess, NULL, 0,
			(LPTHREAD_START_ROUTINE) ::GetProcAddress(hKernel32,
			"FreeLibrary"),
			(void*)loadedDLL, 0, NULL);
		WaitForSingleObject(Temp, INFINITE);
		CloseHandle(Temp);
		CloseHandle(prInf.hProcess);
		res = false;
		ReleaseMutex(Access);
		return;

	}
	if (WaitForSingleObject(Temp, 10000) == WAIT_TIMEOUT)
	{
		TerminateThread(Temp, NO_ERROR);
		WaitForSingleObject(Temp, 1000);
		(*pLog)(LOG_CRITICAL, TEXT("Connection to %d ID Failue: Can't get current dirrectory of proc"), ID);
		VirtualFreeEx(prInf.hProcess, pLib, MAX_PATH_SIZE*sizeof(TCHAR), MEM_RELEASE);
		Temp = CreateRemoteThread(prInf.hProcess, NULL, 0,
			(LPTHREAD_START_ROUTINE) ::GetProcAddress(hKernel32,
			"FreeLibrary"),
			(void*)loadedDLL, 0, NULL);
		WaitForSingleObject(Temp, INFINITE);
		CloseHandle(Temp);
		CloseHandle(prInf.hProcess);
		res = false;
		ReleaseMutex(Access);
		return;
	}
	if (!ReadProcessMemory(prInf.hProcess, pLib, lpCurrDirr, MAX_PATH_SIZE*sizeof(TCHAR), NULL))
	{
		(*pLog)(LOG_CRITICAL, TEXT("Connection to %d ID Failue: Can't get current dirrectory of proc"), ID);
		VirtualFreeEx(prInf.hProcess, pLib, MAX_PATH_SIZE*sizeof(TCHAR), MEM_RELEASE);
		Temp = CreateRemoteThread(prInf.hProcess, NULL, 0,
			(LPTHREAD_START_ROUTINE) ::GetProcAddress(hKernel32,
			"FreeLibrary"),
			(void*)loadedDLL, 0, NULL);
		WaitForSingleObject(Temp, INFINITE);
		CloseHandle(Temp);
		CloseHandle(prInf.hProcess);
		res = false;
		ReleaseMutex(Access);
		return;
	}
	VirtualFreeEx(prInf.hProcess, pLib, MAX_PATH_SIZE*sizeof(TCHAR), MEM_RELEASE);
	CloseHandle(Temp);

	Temp = CreateRemoteThread(prInf.hProcess, NULL, 0,
		(LPTHREAD_START_ROUTINE) ::GetProcAddress(hKernel32,
		"FreeLibrary"),
		(void*)loadedDLL, 0, NULL);
	WaitForSingleObject(Temp, INFINITE);
	CloseHandle(Temp);


	EXITCode = STILL_ACTIVE;
	(*pLog)(LOG_USUALL, TEXT("Successfully connected to %d ID."), ID);
	RegisterWaitForSingleObject(&hWait, prInf.hProcess, RestartCallback, this, INFINITE, WT_EXECUTEONLYONCE);
	ReleaseMutex(Access);
}

