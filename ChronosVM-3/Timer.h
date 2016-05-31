#pragma once

#include <chrono>
#include <thread>
#include <mutex>

class Timer {
	typedef std::chrono::high_resolution_clock clock;
	typedef std::chrono::nanoseconds nanoseconds;

	typedef std::function<void (uint64_t delta, uint64_t total)> callback;
public:
	Timer (double update_rate);
	~Timer ();

	inline void Reset () { this->start = clock::now (); }
	nanoseconds Elapsed () const { return std::chrono::duration_cast<nanoseconds> (clock::now () - this->start); }

	inline void Halt () { this->halted = true; }
	inline void Stop () { this->stop = true; }
	inline void Detach () { this->detached = true; }

	void Start ();
	void Loop ();

	callback Tick = nullptr;
	callback Second = nullptr;
private:
	clock::time_point start;

	double update_rate;
	double ticks_per_cycle;

	double elapsed_ticks = 0;
	double until_sec = 0;
	bool halted = false;

	bool stop = false;
	bool detached = false;

	std::thread *thread;
	std::mutex lock;
};
