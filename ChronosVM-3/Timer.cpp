#include "Timer.h"

Timer::Timer (double update_rate) :
	update_rate (update_rate) {
	this->ticks_per_cycle = clock::period::den / this->update_rate;
}


Timer::~Timer () {
	delete thread;
}

void Timer::Start () {
	this->thread = new std::thread (&Timer::Loop, this);
}

void Timer::Loop () {
	if (this->detached)
		this->thread->detach ();
	while (true) {
		std::lock_guard<std::mutex> (this->lock);

		if (!halted) {
			long long elapsed = this->Elapsed ().count ();

			if (elapsed > ticks_per_cycle) {
				this->until_sec += elapsed;
				this->elapsed_ticks++;

				if (this->Tick != nullptr)
					this->Tick (elapsed, this->elapsed_ticks);

				this->Reset ();
			}
		}

		if (stop)
			if (this->detached) exit (0);
			else if (this->thread->joinable ()) this->thread->join ();
	}
}
