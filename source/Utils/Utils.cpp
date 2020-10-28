#include "Utils.h"
#include <fstream>
#include <cassert>
#include "Windows.h"

namespace fs = std::filesystem;
using namespace std::string_literals;

namespace utils
{
	std::string readFile(std::filesystem::path path)
	{
		if (!fs::exists(path))
			throw std::runtime_error{ "readFile path not found: " +
				fs::absolute(path).string() };
		
		std::ifstream t(path);
		t.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		t.exceptions(std::ifstream::failbit | std::ifstream::badbit);

		t.seekg(0, std::ios::end);
		size_t size = t.tellg();
		std::string buffer(size, ' ');
		t.seekg(0);
		t.read(&buffer[0], size);

		return buffer;
	}

	ThreadPool::~ThreadPool()
	{
		{
			std::unique_lock<std::mutex> lock(queue_mutex);
			stop = true;
		}
		condition.notify_all();
		for (std::thread& worker : workers)
			worker.join();
	}

	int ThreadPool::threadIndex()
	{
		auto findIt = threadIds.find(std::this_thread::get_id());
		if (findIt == threadIds.end())
			throw std::runtime_error{ "Invalid thread for threadIndex" };

		return findIt->second;
	}

	ThreadPool::ThreadPool(size_t threads)
		: stop(false)
	{
		assert(threads);
		for (size_t i = 0; i < threads - 1; ++i)
			workers.emplace_back(
				[this, i]
				{
					{
						std::unique_lock<std::mutex> lock(queue_mutex);
						threadIds.emplace(std::this_thread::get_id(), i);
					}

					for (;;)
					{
						std::function<void()> task;

						{
							std::unique_lock<std::mutex> lock(this->queue_mutex);
							this->condition.wait(lock,
								[this] { return this->stop || !this->tasks.empty(); });
							if (this->stop && this->tasks.empty())
								return;
							task = std::move(this->tasks.front());
							this->tasks.pop();
						}

						task();
					}
				}
				);
	}
}