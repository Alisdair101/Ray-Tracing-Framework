#include "ThreadManager.h"

ThreadManager::ThreadManager()
{
	closeThreads = false;

	for (int i = 0; i < THREADLIMIT; i++)
	{
		threadPool[i] = std::thread(&ThreadManager::ThreadMain, this);
	}
}

ThreadManager::~ThreadManager()
{

}

void ThreadManager::AddTask(std::function<void()> task)
{
	taskList.push(task);
}

void ThreadManager::ThreadMain()
{
	while (!closeThreads)
	{
		if (mutex.try_lock())
		{
			if (taskList.size() > 0)
			{
				if (taskList.front())
				{
					std::function<void()> taskFunction = taskList.front();
					taskList.pop();
					mutex.unlock();

					taskFunction();
				}
			}
			else
			{
				mutex.unlock();
			}
		}
	}
}

void ThreadManager::JoinAllThreads()
{
	while (!closeThreads)
	{
		if (!taskList.empty())
		{
			Sleep(100);
		}
		else
		{
			closeThreads = true;
		}
	}

	for (int i = 0; i < THREADLIMIT; i++)
	{
		if (threadPool[i].joinable())
		{
			threadPool[i].join();
		}
	}
}
