#ifndef ETC_UTILS_H
#define ETC_UTILS_H

#ifndef RAPIDJSON_HAS_STDSTRING
#define RAPIDJSON_HAS_STDSTRING 1
#endif

#include <condition_variable>
#include <mutex>
#include <chrono>

#include <rapidjson/document.h>

// Forward std::make_unique from C++14
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
	return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

namespace minicord
{
	bool net_init();
	bool net_quit();

	std::string stringifyJSON(rapidjson::Value& json);

	class Waiter
	{
	private:
		mutable std::condition_variable cv;
		mutable std::mutex mut;

		bool terminate = false;

	public:
		template<class R, class P>
		bool wait_for(std::chrono::duration<R, P> const &time) const
		{
			std::unique_lock<std::mutex> lock(mut);
			return !cv.wait_for(lock, time, [&] { return terminate; });
		}

		void start()
		{
			std::unique_lock<std::mutex> lock(mut);
			terminate = false;
		}

		void stop()
		{
			std::unique_lock<std::mutex> lock(mut);
			terminate = true;
			cv.notify_all();
		}
	};
}

#endif // ETC_UTILS_H