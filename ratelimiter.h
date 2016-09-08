/*
 * ratelimiter.h
 *
 *  Created on: 2016年9月6日
 *      Author: xie
 */

#ifndef PLUGINS_LIMIT_RATE_RATELIMITER_H_
#define PLUGINS_LIMIT_RATE_RATELIMITER_H_

#include "common.h"
#include <time.h>
#include <vector>
#include <sys/time.h>
#include <pthread.h>
#include <ts/ts.h>
#include <ts/ink_atomic.h>

class LimiterEntry {
public:
	LimiterEntry(uint64_t max_rate, uint64_t milliseconds):
		max_rate_(max_rate), milliseconds_(milliseconds) {
	}
	uint64_t max_rate(){
		return max_rate_;
	}
	uint64_t milliseconds(){
		return milliseconds_;
	}
private:
	const uint64_t max_rate_;
	const uint64_t milliseconds_;
	DISALLOW_COPY_AND_ASSIGN(LimiterEntry);
};

class LimiterState{
public:
	explicit LimiterState(float allowances[], timeval times[]):
			allowances_(allowances), times_(times), taken_(NULL) {
		taken_ = (float *) malloc(sizeof(allowances));
		memset(taken_, 0, sizeof(float) * (sizeof(allowances) / sizeof( float )));
	}

	float allowance(int index) {
		return allowances_[index];
	}
	void set_allowance(int index, float x) {
		allowances_[index] = x;
	}

	timeval time(int index) {
		return times_[index];
	}
	void set_time(int index, timeval x) {
		times_[index] = x;
	}

	float taken(int index) {
		return taken_[index];
	}
	void set_taken(int index, float amount) {
		taken_[index] += amount;
	}

	~LimiterState() {
		free(allowances_);
		free(times_);
		free(taken_);
	}
private:
	float * allowances_;
	timeval * times_;
	float * taken_;
	DISALLOW_COPY_AND_ASSIGN(LimiterState);
};


class RateLimiter {
public:
	explicit RateLimiter():
		enable_limit(false), _ref_count(0), update_mutex_(TSMutexCreate()) {
		int rc = pthread_rwlock_init(&rwlock_keymap_, NULL);
		TSReleaseAssert(!rc);
	}
	~RateLimiter() {
		for(size_t i = 0; i < counters_.size(); i++) {
			delete counters_[i];
			counters_[i] = NULL;
		}
		pthread_rwlock_destroy(&rwlock_keymap_);
	}

//	void hold() {
//	   ink_atomic_increment(&_ref_count, 1);
//	   TSDebug(PLUGIN_NAME,"----------hold  _ref_count---------------%d",_ref_count);
//	}
//
//	void release() {
//	   if (1 >= ink_atomic_decrement(&_ref_count, 1)) {
//		   TSDebug(PLUGIN_NAME,"----------release  _ref_count---------------%d",_ref_count);
//		   delete this;
//	   }
//	}

	int addCounter(float max_rate, uint64_t milliseconds);
	uint64_t getMaxUnits(uint64_t amount, LimiterState * state);
	LimiterState * registerLimiter();
	LimiterEntry * getCounter(int index);

	bool getEnableLimit() { return enable_limit; }
	void setEnableLimit(bool onlimit) { enable_limit = onlimit; }

private:
	float * getCounterArray();
	timeval * getTimevalArray(timeval tv);
	bool enable_limit;

	std::vector<LimiterEntry *> counters_;
	pthread_rwlock_t rwlock_keymap_;
	TSMutex update_mutex_;
	volatile int _ref_count;
	DISALLOW_COPY_AND_ASSIGN(RateLimiter);
};


#endif /* PLUGINS_LIMIT_RATE_RATELIMITER_H_ */
