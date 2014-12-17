#pragma once
#include <Windows.h>
#include <cstdlib>
#include <vector>
#include <ctime>
#include <deque>
#include <fstream>
#define MAXBUFFERSIZE 1000
#define LOG_CRITICAL 2
#define LOG_USUALL 1

class Logger
{
public:
	Logger(){};
	virtual void operator() (int,  LPTSTR outputmessage, ...)=0;
	virtual ~Logger();
};



class TLogger :public Logger
{
private:
	HANDLE LockPrint;									//Mutex for printing
	HANDLE RunningThread;								//Thread for Run function
	HANDLE LockQueue;									//Mutex for queue access
	HANDLE runAllert;									//Event to signal Run function
	DWORD RunningThreadId;								//
	virtual bool Write(LPCTSTR) = 0;					//Write this function for your logger. Function must write smth to your log
	LPTSTR Analys(time_t,LPCTSTR, va_list);				//Analys of the string and inserting arguments
	std::deque<LPTSTR> Log;								//deque of messages
	static friend DWORD WINAPI Run(LPVOID);				//Function that get messages from deque and write them with Write func
	bool running;										//Flag for stopping
	int LogMode;										//Log mode (deafult critical only)
	void Push(LPTSTR);									//Pushing string into deque
public:
	void Stop();										//Stopping Run function
	void SetMode(int);									//Set logger mode(1 - all events, 2 - only critical)
	int GetMode();										//Get logger mode int
	void operator() (int LogMode, LPTSTR outputmessage, ...);//Main function
	TLogger();											//
	virtual ~TLogger();									//
};


class MyLogger :public TLogger
{
private:
	std::wostream &outstream;							//output stream
	bool Write(LPCTSTR);								//writing to output stream
public:
	MyLogger(std::wostream &of) :outstream(of){};
	~MyLogger();
};

LPTSTR IntToTChar(int i);								//making i string