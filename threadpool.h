#include <cstdint>
#include <string>
#include <chrono>
#include <mutex>
#include <queue>
#include <optional>
#include <thread>
#include <functional>

#include "SBSObjects.h"

struct JobMessage {

	uint64_t job_id = 0;
	std::string payload;
	std::chrono::steady_clock::time_point timestamp;
};

class SharedQueue {
private:
	std::condition_variable cond_var;
	std::mutex queue_mutex;
	std::queue<JobMessage> m_queue;
	std::atomic<bool> stopping = false;

public:
	SharedQueue() {};
	~SharedQueue() {

		stopping = true;
		cond_var.notify_all();

	};

	void enqueue_job(JobMessage job) {
		{
			std::unique_lock<std::mutex> lock(queue_mutex);
			m_queue.push(job);
		}
		cond_var.notify_one();
	}

	std::optional<JobMessage> dequeue_job() {
		std::unique_lock<std::mutex> lock(queue_mutex);
		cond_var.wait(lock, [this] { return !m_queue.empty() || stopping; });
		if (stopping && m_queue.empty()) {
			return std::nullopt;
		}
		else {
			JobMessage job = m_queue.front();
			m_queue.pop();
			return job;
		}
	}

};

class Thread_pool {
private:
	std::vector<std::thread> threads;
	SharedQueue& queue;
	std::function<void(const JobMessage&)> on_job;

public:

	Thread_pool(SharedQueue& queue, int thread_count, std::function<void(const JobMessage&)> handler) : queue(queue), on_job(handler) {
		for (int i = 0; i < thread_count; i++) {
			threads.emplace_back(&Thread_pool::worker_loop, this);
		}
	};

	~Thread_pool() {

		for (auto& thread : threads) {
			thread.join();
		}

	};

	void worker_loop() {
		while (true) {
			std::optional<JobMessage> job = this->queue.dequeue_job();
			if (!job) {
				return;
			}
			on_job(*job);
		}
	}

};