#pragma once
#include <queue>
#include <mutex>
#include <iostream>

template<class T>
class blocking_queue
{

private:
	int max_size_;
	int size_;
	std::mutex m_;
	std::condition_variable space_avaible, element_avaible;
	std::queue<T> queue_;


public:
	blocking_queue() : max_size_(INT_MAX), size_(0) {};
	explicit blocking_queue(const int size) : max_size_(size), size_(0) {};

	bool add(T t);
	bool contains(T t);
	void put(T t);
	T take();
	int size();

};

template <class T>
bool blocking_queue<T>::add(T t)
{
	std::lock_guard<std::mutex> lg(m_);

	if (size_ >= max_size_) return false;

	size_++;
	queue_.push(t);
	element_avaible.notify_one();
	return true;
}

template <class T>
bool blocking_queue<T>::contains(T t)
{
	std::lock_guard<std::mutex> lg(m_);

	for (auto it = queue_._Get_container().begin(); it != queue_._Get_container().end(); ++it)
		if (*it == t) return true;

	return false;
}

template <class T>
void blocking_queue<T>::put(T t)
{
	std::unique_lock<std::mutex> ul(m_);

	if (size_ >= max_size_)
		space_avaible.wait(ul, [this]() {return size_ < max_size_; });

	size_++;
	queue_.push(t);
	element_avaible.notify_one();
}

template <class T>
T blocking_queue<T>::take()
{
	std::unique_lock<std::mutex> ul(m_);

	if (size_ == 0)
		element_avaible.wait(ul, [this]() {return size_ > 0; });

	T ret = queue_.front();
	queue_.pop();
	size_--;
	space_avaible.notify_one();
	return ret;
}

template <class T>
int blocking_queue<T>::size()
{
	std::lock_guard<std::mutex> lg(m_);
	return size_;
}
