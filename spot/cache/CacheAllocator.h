#ifndef SPOT_CACHE_CACHE_ALLOCATOR_H
#define SPOT_CACHE_CACHE_ALLOCATOR_H
#include <spot/utility/Compatible.h>
#include<string>
class CacheAllocator
{
public:
	CacheAllocator();
	~CacheAllocator();
	static CacheAllocator* instance();
	char* allocate(uint32_t length);		
	void print();
private:
	char *cachePtrL1_;
	char *cachePtrL2_;
	char *cachePtrL3_;
	uint32_t readIndexL1_;
	uint32_t readIndexL2_;
	uint32_t readIndexL3_;
};
#endif  

