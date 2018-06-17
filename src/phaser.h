#pragma once
#include <mutex>

class phaser
{
	enum phaser_states
	{
		terminated = -1,
		running
	};
	std::mutex m_;
	std::condition_variable advancer_;
	phaser_states state_ = running;
	int registered_;
	int future_phase_registered_;
	int phase_number_ = 0;
	int arrived_parties_ = 0;
	void advance();

public:
	phaser() : registered_(0), future_phase_registered_(0) {};
	explicit phaser(const int parties) : registered_(parties), future_phase_registered_(parties) {};

	int arrive();
	int arrive_and_await_advance();
	int arrive_and_deregister();
	int await_advance(int phase);
	int bulk_register(int parties);
	void force_termination();
	int get_arrived_parties();
	int get_phase();
	int get_registered_parties();
	int get_unarrived_parties();
	bool is_terminated();
	int register_one();
};

inline int phaser::register_one()
{
	std::lock_guard<std::mutex> ul(m_);
	if (state_ == terminated)
		return -1;
	registered_++;
	future_phase_registered_++;
	return phase_number_;
}

inline int phaser::bulk_register(const int parties)
{
	std::lock_guard<std::mutex> ul(m_);
	if (state_ == terminated)
		return -1;
	registered_ += parties;
	future_phase_registered_ += parties;
	return phase_number_;
}

inline void phaser::advance()
{
	phase_number_++;
	registered_ = future_phase_registered_;
	arrived_parties_ = 0;
	advancer_.notify_all();
}

inline int phaser::arrive()
{
	std::lock_guard<std::mutex> lg(m_);
	if (state_ == terminated)
		return -1;
	arrived_parties_++;
	const auto ret = phase_number_;
	if (arrived_parties_ >= registered_)
		advance();
	return ret;
}

inline int phaser::arrive_and_await_advance()
{
	std::unique_lock<std::mutex> ul(m_);
	if (state_ == terminated)
		return -1;
	arrived_parties_++;
	const auto ret = phase_number_;
	if (arrived_parties_ >= registered_)
		advance();
	else
	{
		advancer_.wait(ul, [this, &ret]() { return state_ == terminated || ret < phase_number_; });
		if (state_ == terminated) return -1;
	}
	return ret;
}

inline int phaser::arrive_and_deregister()
{
	std::lock_guard<std::mutex> lg(m_);
	if (state_ == terminated)
		return -1;
	arrived_parties_++;
	const auto ret = phase_number_;
	future_phase_registered_--;
	if (arrived_parties_ >= registered_)
		advance();
	return ret;
}

inline int phaser::await_advance(int phase)
{
	std::unique_lock<std::mutex> ul(m_);
	if (state_ == terminated || phase_number_!=phase)
		return -1;
	if (arrived_parties_ >= registered_)
		advance();
	else
	{
		advancer_.wait(ul, [this, &phase]() { return state_ == terminated || phase < phase_number_; });
		if (state_ == terminated) return -1;
	}
	return phase_number_;
}

inline void phaser::force_termination()
{
	std::lock_guard<std::mutex> ul(m_);
	state_ = terminated;
	phase_number_ = -1;
}

inline int phaser::get_phase()
{
	std::lock_guard<std::mutex> ul(m_);
	if (state_ == terminated) return -1;
	return phase_number_;
}

inline int phaser::get_registered_parties()
{
	std::lock_guard<std::mutex> ul(m_);
	return registered_;
}

inline int phaser::get_arrived_parties()
{
	std::lock_guard<std::mutex> ul(m_);
	return arrived_parties_;
}

inline int phaser::get_unarrived_parties()
{
	std::lock_guard<std::mutex> ul(m_);
	return registered_ - arrived_parties_;
}

inline bool phaser::is_terminated()
{
	std::lock_guard<std::mutex> ul(m_);
	return state_ == terminated;
}