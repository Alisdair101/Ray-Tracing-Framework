#pragma once

#include <thread>
#include <vector>
#include <condition_variable>
#include <queue>

#include "windows.h"

#define THREADLIMIT 8

class ThreadManager
{
public:
	ThreadManager();
	~ThreadManager();

	void AddTask(std::function<void()> task);

	void ThreadMain();
	void JoinAllThreads();

private:
	std::queue<std::function<void()>> taskList;
	std::thread threadPool[THREADLIMIT];
	std::mutex mutex;

	bool closeThreads;
};

