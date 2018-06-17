#pragma once
#include <mutex>
template<class T>
class exchanger
{
	std::mutex m_;
	std::condition_variable avaibility_;
	T stored_;
	bool avaible_ = false;
	bool exchanged_ = false;

public:
	T exchange(T t);
	template <typename Rep, typename Period>
	T exchange(T t, std::chrono::duration<Rep, Period> timeout);
};

template <class T>
T exchanger<T>::exchange(const T t)
{
	std::unique_lock<std::mutex> ul(m_);
	if (avaible_)
	{
		T temp = stored_;
		stored_ = t;
		exchanged_ = true;
		avaibility_.notify_one();
		return temp;
	}

	stored_ = t;
	avaible_ = true;
	exchanged_ = false;
	avaibility_.notify_one();
	avaibility_.wait(ul, [this] { return exchanged_; });
	avaible_ = false;
	return stored_;
}

template <class T>
template <typename Rep, typename Period>
T exchanger<T>::exchange(const T t, std::chrono::duration<Rep, Period> timeout)
{
	std::unique_lock<std::mutex> ul(m_);
	if (avaible_)
	{
		T temp = stored_;
		stored_ = t;
		exchanged_ = true;
		avaibility_.notify_one();
		return temp;
	}

	stored_ = t;
	avaible_ = true;
	exchanged_ = false;
	avaibility_.notify_one();

	auto ts = std::chrono::system_clock::now();
	avaibility_.wait_for(ul, timeout, [this, ts, timeout]()
	{
		return exchanged_ || (std::chrono::system_clock::now() - ts) > timeout;
	});

	avaible_ = false;
	if (exchanged_ == false) throw std::exception("timeout exception");
	return stored_;
}
