//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <deque>

template<typename T>
class ThreadedQueue {
public:
	std::function<void(const T&)> onOutput;
	std::deque<T> queue;

	std::mutex mutex;
	std::thread* thread = nullptr;
	std::condition_variable cv;
	std::atomic_bool running;

	void stop() {
		if (thread) {
			running = false;
			cv.notify_all();
			if (thread->joinable()) {
				thread->join();
			}
			else {
				thread->detach();
			}
			delete thread;
			thread = nullptr;
		}
	}

	void start() {
		stop();
		running = true;
		thread = new std::thread([&]() {
			while (running) {
				std::unique_lock<std::mutex> lock(mutex);
				if (queue.empty()) {
					cv.wait(lock);
				}
				else {
					T value = queue.front();
					queue.pop_front();
					lock.unlock();
					if (onOutput) {
						onOutput(value);
					}
				}
			}
			});
	}

	bool empty() {
		return queue.empty();
	}

	void onInput(const T& value) {
		std::unique_lock<std::mutex> lock(mutex);
		queue.push_back(value);
		cv.notify_one();
	}
};