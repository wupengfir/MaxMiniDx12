#pragma once
#include "Common.h"
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
class WorkThreadPool
{
private:
	static thread_local int m_thread_Index;
	std::mutex m_queueMutex;
	std::condition_variable m_condition;
	bool m_stop = false;
	std::vector<std::thread> m_threads{};
	std::queue<std::function<void(int)>> m_tasks;
	std::atomic<UINT> m_taskCount{ 0 };
public:
	explicit WorkThreadPool(int threadCount);
	~WorkThreadPool();

	void EnqueueTask(std::function<void(int)> task);
	UINT GetTaskCount() const { return m_taskCount.load(std::memory_order_relaxed); }
};