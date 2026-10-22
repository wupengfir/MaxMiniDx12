#include "WorkThreadPool.h"

thread_local int WorkThreadPool::m_thread_Index = -1;
WorkThreadPool::WorkThreadPool(int threadCount)
{
	for (size_t i = 0; i < threadCount; i++)
	{
		m_threads.emplace_back(std::thread([this,i]
			{
				m_thread_Index = i;
				std::function<void(int)> task;
				while (true)
				{
					{
						std::unique_lock<std::mutex> lock(m_queueMutex);
						m_condition.wait(lock, [this] 
							{ 
								return !m_tasks.empty() || m_stop; 
							});
						if (m_stop && m_tasks.empty())
							return;
						task = std::move(m_tasks.front());
						m_tasks.pop();
					}
					task(m_thread_Index);
					m_taskCount.fetch_add(-1, std::memory_order_acquire);
				}
			}));
	}
}

WorkThreadPool::~WorkThreadPool()
{
	m_stop = true;

	m_condition.notify_all();
	for (auto& t : m_threads)
	{
		t.join();
	}
}

void WorkThreadPool::EnqueueTask(std::function<void(int)> task)
{
	{
		std::unique_lock<std::mutex> lock(m_queueMutex);
		m_tasks.push(std::move(task));
		m_taskCount.fetch_add(1, std::memory_order_relaxed);
	}
	m_condition.notify_one();
}
