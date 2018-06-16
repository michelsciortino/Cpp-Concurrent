#pragma once
#include <mutex>
#include <chrono>

class cyclic_barrier
{
	enum barrier_state
	{
		broken = -1,
		initialized,
		waiting
	};

private:
	int parties_, waiting_parties_;
	barrier_state state;
	std::mutex m_;
	std::condition_variable cv_;

public:
	explicit cyclic_barrier(const int parties) : parties_(parties), waiting_parties_(0), state(initialized) {};
	int await();									//Waits until all parties have invoked await on this barrier.
	template <typename Rep, typename Period>		//Waits until all parties have invoked await on this barrier, or the specified waiting time elapses.
	int await(std::chrono::duration<Rep,Period> timeout);
	int get_number_waiting();						//Returns the number of parties currently waiting at the barrier.
	int get_parties();								//Returns the number of parties required to trip this barrier.
	bool is_broken();								//Queries if this barrier is in a broken state.
	void reset();
};

inline int cyclic_barrier::await()
{
	std::unique_lock<std::mutex> ul(m_);
	if (state == broken)
		throw std::exception("broken_barrier_exception");
	if (state == initialized) state = waiting;

	const auto ret = parties_ - waiting_parties_;
	waiting_parties_++;

	if (waiting_parties_ < parties_)
		cv_.wait(ul, [this]() {return (waiting_parties_ >= parties_) || (state != waiting); });
	else
	{
		state = broken;
		cv_.notify_all();
	}

	if (state == initialized)
		throw std::exception("broken_barrier_exception");
	return ret;
}

template <typename Rep, typename Period>
inline int cyclic_barrier::await(const std::chrono::duration<Rep,Period> timeout)
{
	std::unique_lock<std::mutex> ul(m_);
	if (state == broken)
		throw std::exception("broken_barrier_exception");
	if (state == initialized) state = waiting;

	const auto ret = parties_ - waiting_parties_;
	waiting_parties_++;

	if (waiting_parties_ < parties_)
		cv_.wait_for(ul, timeout, [this]() {return (waiting_parties_ >= parties_) || (state != waiting); });
	else
	{
		state = broken;
		cv_.notify_all();
	}

	if (state == initialized)
		throw std::exception("broken_barrier_exception");
	if(waiting_parties_<parties_)
	{
		waiting_parties_--;
		throw std::exception("timeout_exception");
	}

	return ret;
}

inline int cyclic_barrier::get_number_waiting()
{
	std::lock_guard<std::mutex> lg(m_);
	return waiting_parties_;
}

inline int cyclic_barrier::get_parties()
{
	std::lock_guard<std::mutex> lg(m_);
	return parties_;
}

inline bool cyclic_barrier::is_broken()
{
	std::lock_guard<std::mutex> lg(m_);
	return state == broken;
}

inline void cyclic_barrier::reset()
{
	std::lock_guard<std::mutex> lg(m_);
	state = initialized;
	waiting_parties_=0;
	cv_.notify_all();
}