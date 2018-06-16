#pragma once
#include <queue>
#include <mutex>
#include <iostream>

template <class T, class Container = std::vector<T>,class Compare = std::less<typename Container::value_type>> 
class priority_blocking_queue
{

private:
	int max_size_;
	int size_;
	std::mutex m_;
	std::condition_variable space_avaible, element_avaible;
	std::priority_queue<T,Container,Compare> priority_queue_;


public:
	priority_blocking_queue() : max_size_(INT_MAX), size_(0) {};
	explicit priority_blocking_queue(const int size) : max_size_(size), size_(0) {};

	bool add(T t);
	bool contains(T t);
	void put(T t);
	T take();
	int size();

};

template <class T, class Container,class Compare>
bool priority_blocking_queue<T,Container,Compare>::add(T t)
{
	std::lock_guard<std::mutex> lg(m_);

	if (size_ >= max_size_) return false;

	size_++;
	priority_queue_.push(t);
	element_avaible.notify_one();
	return true;
}

template <class T, class Container,class Compare>
bool priority_blocking_queue<T,Container,Compare>::contains(T t)
{
	std::lock_guard<std::mutex> lg(m_);

	for (auto it = priority_queue_.begin(); it != priority_queue_.end(); ++it)
		if (*it == t) return true;

	return false;
}

template <class T, class Container,class Compare>
void priority_blocking_queue<T,Container,Compare>::put(T t)
{
	std::unique_lock<std::mutex> ul(m_);

	if (size_ >= max_size_)
		space_avaible.wait(ul, [this]() {return size_ < max_size_; });

	size_++;
	priority_queue_.push(t);
	element_avaible.notify_one();
}

template <class T, class Container,class Compare>
T priority_blocking_queue<T,Container,Compare>::take()
{
	std::unique_lock<std::mutex> ul(m_);

	if (size_ == 0)
		element_avaible.wait(ul, [this]() {return size_ > 0; });

	T ret = priority_queue_.top();
	priority_queue_.pop();
	size_--;
	space_avaible.notify_one();
	return ret;
}

template <class T, class Container,class Compare>
int priority_blocking_queue<T,Container,Compare>::size()
{
	std::lock_guard<std::mutex> lg(m_);
	return size_;
}