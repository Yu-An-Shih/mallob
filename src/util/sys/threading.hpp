
#ifndef DOMPASCH_MALLOB_THREADING_HPP
#define DOMPASCH_MALLOB_THREADING_HPP

#include <functional>
#include <mutex>
#include <condition_variable>

class Mutex {
private:
	std::mutex mtx;
    
public:
	void lock();
	void unlock();
	std::unique_lock<std::mutex> getLock();    // blocks until acquiring the lock
	bool tryLock();
};

class ConditionVariable {
private:
	std::condition_variable condvar;
    
public:
    void wait(Mutex& mutex, std::function<bool()> condition);    // 1. blocks until aquiring mutex, and 
	                                                             // 2. release mutex and wait until condition is met
	void waitWithLockedMutex(std::unique_lock<std::mutex>& lock, std::function<bool()> condition); // release mutex and wait until condition is met
	void notifySingle();
	void notify();
};

#endif 
