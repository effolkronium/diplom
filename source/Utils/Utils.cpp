#include "Utils.h"
#include <fstream>
#include <cassert>

#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/time.h>
#include <sys/resource.h>
#endif

namespace fs = std::filesystem;
using namespace std::string_literals;


namespace
{
	#define WINDOWS_TICK 10000000
	#define SEC_TO_UNIX_EPOCH 11644473600LL

	unsigned long long  WindowsTickToUnixSeconds(long long windowsTicks)
	{
		return static_cast<unsigned long long>(windowsTicks / WINDOWS_TICK - SEC_TO_UNIX_EPOCH);
	}
}


namespace utils
{

#ifdef _WIN32
	unsigned long long getThreadSeconds()
	{
		FILETIME crTime{}, exTime{}, kernTime{}, userTime{};
		if (0 == GetThreadTimes(GetCurrentThread(), &crTime, &exTime, &kernTime, &userTime))
			throw std::runtime_error{ "GetThreadTimes error: "s + std::to_string(GetLastError()) };



		return WindowsTickToUnixSeconds(ULARGE_INTEGER{ kernTime.dwLowDateTime, kernTime.dwHighDateTime }.QuadPart)
			+ WindowsTickToUnixSeconds(ULARGE_INTEGER{ userTime.dwLowDateTime, userTime.dwHighDateTime }.QuadPart);
	}
#else
    unsigned long long getThreadSeconds()
	{
		struct rusage usage;
		getrusage(RUSAGE_THREAD, &usage);
		return usage.ru_utime.tv_sec + usage.ru_stime.tv_sec;
	}
#endif

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
		for (size_t i = 0; i < threads; ++i)
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
