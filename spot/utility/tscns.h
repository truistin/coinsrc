#ifndef __SPOT_UTILITY_TSCNS_H__
#define __SPOT_UTILITY_TSCNS_H__
#ifdef __LINUX__
#include <time.h>
#include<thread>
class TSCNS
{
public:
	static TSCNS& instance()
	{
		static TSCNS instance_;
		return instance_;
	}
	void init()
	{
		init(0.0);
		std::this_thread::sleep_for(std::chrono::seconds(10));
		calibrate();
	}
	inline uint64_t rdns() const 
	{ 
		return tsc2ns(rdtsc()); 
	}
	inline uint64_t rdus() const
	{
		return tsc2ns(rdtsc()) / 1000;
	}
	inline uint64_t rdms() const
	{
		return rdus() / 1000;
	}
	inline uint64_t rds() const
	{
		return rdms() / 1000;
	}
	void init(double tsc_ghz) 
	{
		syncTime(base_tsc, base_ns);
		if (tsc_ghz <= 0.0) return;
		tsc_ghz_inv = 1.0 / tsc_ghz;
		adjustOffset();
	}

	double calibrate() 
	{
		uint64_t delayed_tsc, delayed_ns;
		syncTime(delayed_tsc, delayed_ns);
		tsc_ghz_inv = (double)(int64_t)(delayed_ns - base_ns) / (int64_t)(delayed_tsc - base_tsc);
		adjustOffset();
		return 1.0 / tsc_ghz_inv;
	}

	// You can change to using rdtscp if ordering is important
	inline uint64_t rdtsc() const { return __builtin_ia32_rdtsc(); }

	inline uint64_t tsc2ns(uint64_t tsc) const { return ns_offset + (int64_t)((int64_t)tsc * tsc_ghz_inv); }

	

	// If you want cross-platform, use std::chrono as below which incurs one more function call:
	// return std::chrono::high_resolution_clock::now().time_since_epoch().count();
	uint64_t rdsysns() const {
		timespec ts;
		::clock_gettime(CLOCK_REALTIME, &ts);
		return ts.tv_sec * 1000000000 + ts.tv_nsec;
	}

	// For checking purposes, see test.cc
	uint64_t rdoffset() const { return ns_offset; }

private:
	// Linux kernel sync time by finding the first try with tsc diff < 50000
	// We do better: we find the try with the mininum tsc diff
	void syncTime(uint64_t& tsc, uint64_t& ns) {
		const int N = 10;
		uint64_t tscs[N + 1];
		uint64_t nses[N + 1];

		tscs[0] = rdtsc();
		for (int i = 1; i <= N; i++) {
			nses[i] = rdsysns();
			tscs[i] = rdtsc();
		}

		int best = 1;
		for (int i = 2; i <= N; i++) {
			if (tscs[i] - tscs[i - 1] < tscs[best] - tscs[best - 1]) best = i;
		}
		tsc = (tscs[best] + tscs[best - 1]) >> 1;
		ns = nses[best];
	}

	void adjustOffset() { ns_offset = base_ns - (int64_t)((int64_t)base_tsc * tsc_ghz_inv); }

	alignas(64) double tsc_ghz_inv = 1.0; // make sure tsc_ghz_inv and ns_offset are on the same cache line
	uint64_t ns_offset = 0;
	uint64_t base_tsc = 0;
	uint64_t base_ns = 0;
};
#endif
#endif