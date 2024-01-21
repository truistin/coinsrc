#ifndef SPOT_CACHE_CACHENODE_H
#define SPOT_CACHE_CACHENODE_H
#include <spot/utility/Compatible.h>
#include<string>
class CacheNode
{
public:
	CacheNode(uint32_t size,uint32_t index,std::string bufferName);
	~CacheNode();
	char* allocate(uint32_t length);	
	void release(uint32_t length);
	CacheNode* getNext();
	void setNext(CacheNode* next);
	void reset();
	void print();
	inline uint32_t availableSize();
	inline uint32_t readalbeSize();
	inline uint32_t bufferSize();
	inline uint32_t index();
	std::string& bufferName();
private:
	uint32_t bufferSize_;
	uint32_t readIndex_;
	uint32_t writeIndex_;
	uint32_t index_;
	char *headPtr_;
	CacheNode *next_;
	std::string bufferName_;
};
inline uint32_t CacheNode::availableSize()
{
	return bufferSize_ - writeIndex_;
}
inline uint32_t CacheNode::bufferSize()
{
	return bufferSize_;
}
inline uint32_t CacheNode::readalbeSize()
{
	return writeIndex_ - readIndex_;
}
inline uint32_t CacheNode::index()
{
	return index_;
}
#endif  

