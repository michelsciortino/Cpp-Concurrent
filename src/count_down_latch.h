#pragma once
#include <mutex>
#include <chrono>

class count_down_latch
{
private:
	std::mutex m_;
	std::condition_variable cv_;
	long count_;
public:
	explicit count_down_latch(const int count) :count_(count) {};
	~count_down_latch();
	void await();											//Causes the current thread to wait until the latch has counted down to zero.
	template <typename Rep, typename Period>
	bool await(std::chrono::duration<Rep, Period> timeout);	//Causes the current thread to wait until the latch has counted down to zero, unless the specified waiting time elapses.
	void count_down();										//Decrements the count of the latch, releasing all waiting threads if the count reaches zero.
	long count();											//Returns the current count.
};

inline count_down_latch::~count_down_latch()
{
	std::lock_guard<std::mutex> lg(m_);
	count_ = 0;
	cv_.notify_all();
}

inline void count_down_latch::await()
{
	std::unique_lock<std::mutex> ul(m_);
	if (count_ <= 0) return;
	cv_.wait(ul, [this]() {return count_ == 0; });
}

template <typename Rep, typename Period>
bool count_down_latch::await(std::chrono::duration<Rep, Period> timeout)
{
	std::unique_lock<std::mutex> ul(m_);
	if (count_ <= 0) return true;
	cv_.wait_for(ul, timeout, [this]() {return count_ == 0; });
	if (count != 0)
		return false;
	return true;
}

inline void count_down_latch::count_down()
{
	std::lock_guard<std::mutex> lg(m_);
	count_--;
	if (count_ <= 0) cv_.notify_all();
}

inline long count_down_latch::count()
{
	std::lock_guard<std::mutex> lg(m_);
	return count_;
}
