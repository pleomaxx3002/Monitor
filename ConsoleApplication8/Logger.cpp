#include "stdafx.h"
#include "Logger.h"



Logger::~Logger()
{
}


TLogger::TLogger()
{
	LockPrint = CreateMutex(NULL, FALSE, NULL);
	LockQueue = CreateMutex(NULL, FALSE, NULL);
	runAllert = CreateEvent(NULL, TRUE, FALSE, NULL);
	running = false;
	LogMode = 2;
}

TLogger::~TLogger()
{
	Stop();
	CloseHandle(LockPrint);
	CloseHandle(LockQueue);
	CloseHandle(RunningThread);
}

void TLogger::operator()(int MessPr, LPTSTR output_message, ...)
{
	if (MessPr < LogMode)
		return;
	va_list p;
	va_start(p, output_message);
	time_t event_time = time(NULL);
	LPTSTR temp = Analys(event_time, output_message, p);
	Push(temp);
	free(temp);
	va_end(p);
	if (!running)
	{
		running = true;
		DWORD exitcode=0;
		GetExitCodeThread(RunningThread, &exitcode);
		if (exitcode != STILL_ACTIVE)
			RunningThread = CreateThread(NULL, 0, Run, this, 0, &RunningThreadId);
	}
}

int TLogger::GetMode()
{
	return LogMode;
}

void TLogger::SetMode(int i)
{
	if (i < 0)
		return;
	if (i>2)
		return;
	LogMode = i;

}


void TLogger::Stop()
{
	if (!running)
		return;
	do
	{
		running = false;
		SetEvent(runAllert);
	} while (WaitForSingleObject(RunningThread, 1000) == WAIT_TIMEOUT);
}

void TLogger::Push(LPTSTR str)
{
	WaitForSingleObject(LockQueue, INFINITE);
	Log.push_back(_tcsdup(str));
	SetEvent(runAllert);
	ReleaseMutex(LockQueue);
}

LPTSTR TLogger::Analys(time_t Time,LPCTSTR source, va_list List)
{
	TCHAR *res=new TCHAR[MAXBUFFERSIZE];
	_tctime_s(res, 26, &Time);
	res[24] = _T(':');
	int ID = GetCurrentProcessId();
	TCHAR p[10];
	int i = 0;
	while (ID > 0)
		p[i++] = ID % 10 + _T('0'), ID /= 10;
	int j;
	for (j = 25; i > 0; i--, j++)
		res[j] = p[i - 1];
	res[j] = _T('\t');
	res[j+1] = _T('\0');
	for (int i = 0, N = _tcslen(source), k = j + 1; i < N; i++)
	{
		if (i < N - 1 && source[i] == _T('%'))
		{
			i++;
			switch (source[i])
			{
			case _T('d'):
			{
							LPTSTR temp = IntToTChar(va_arg(List, int));
							_tcscat_s(res, MAXBUFFERSIZE, temp);
							k = _tcslen(res);
							res[k] = _T('\0');
							break;
			}
			case _T('S'):
			{
							LPTSTR temp = va_arg(List, LPTSTR);
							_tcscat_s(res, MAXBUFFERSIZE, temp);
							k = _tcslen(res);
							res[k] = _T('\0');
							break;
			}
			}
		}
		else
			res[k] = source[i], k++, res[k] = _T('\0');
	}
	return res;
}


LPTSTR IntToTChar(int x)
{
	if (x == 0)
		return _T("0");
	short sgn = (x < 0) ? 1 : 0;
	int n = sgn;
	int temp = abs(x);
	while (temp>0)
		temp = temp / 10, n++;
	temp = abs(x);
	TCHAR *p = new TCHAR[n + 1];
	for (int i = n - 1; i >= sgn; --i)
	{
		p[i] = temp % 10 + _T('0');
		temp = temp / 10;
	}
	if (sgn == 1) p[0] = _T('-');
	p[n] = _T('\0');
	return p;
}

DWORD WINAPI Run(LPVOID p)
{
	TLogger *This = (TLogger*)p;
	while (This->running)
	{
		WaitForSingleObject(This->runAllert, INFINITE);
		ResetEvent(This->runAllert);
		LPTSTR temp;
		WaitForSingleObject(This->LockQueue, INFINITE);
		while (!This->Log.empty())
		{
			temp = _tcsdup(This->Log.front());
			free(This->Log.front());
			This->Log.pop_front();
			ReleaseMutex(This->LockQueue);
			WaitForSingleObject(This->LockPrint, INFINITE);
			This->Write(temp);
			ReleaseMutex(This->LockPrint);
			free(temp);
			WaitForSingleObject(This->LockQueue, INFINITE);
		}
		ReleaseMutex(This->LockQueue);
	}
	return 0;
}


bool MyLogger::Write(LPCTSTR str)
{
	for (int i = 0, n = _tcslen(str); i < n; i++)
		outstream << str[i];
	outstream << std::endl;
	return true;
}

MyLogger::~MyLogger()
{
	
}

