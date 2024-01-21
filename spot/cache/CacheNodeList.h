#ifndef SPOT_CACHE_CACHENODELIST_H
#define SPOT_CACHE_CACHENODELIST_H
#include <spot/cache/CacheNode.h>
#include<atomic>
class CacheNodeList
{
public:
	CacheNodeList(std::string bufferName,uint32_t nodeSize = 16 * 1024 * 1024);
	~CacheNodeList();
	char* allocate(uint32_t length);
	void release(uint32_t length);
	inline uint32_t nodeCount();
	inline uint32_t allNodeSize();	
	CacheNode* newCacheNode();
	std::string& bufferName();
	void printNodeList();
private:
	CacheNode *headNode_;
	CacheNode *currAllocateNode_;
	CacheNode *currReleaseNode_;
	uint32_t nodeSize_;
	uint32_t nodeCount_;		
};

inline uint32_t CacheNodeList::nodeCount()
{
	return nodeCount_;
}
inline uint32_t CacheNodeList::allNodeSize()
{
	return nodeCount_*nodeSize_;
}

#endif  

