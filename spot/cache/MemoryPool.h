#ifndef SPOT_MEMORY_POOL_H
#define SPOT_MEMORY_POOL_H
#include "spot/utility/Logging.h"
#include <climits>
#include <cstddef>
#include <string>
using namespace spot::utility;
template <typename T, size_t BlockSize = 128*sizeof(T)>
class MemoryPool
{
public:
	union Slot
	{
		T element;
		Slot* next;
	};	

	MemoryPool()
	{		
		currentBlock_ = nullptr;
		currentSlot_ = nullptr;
		lastSlot_ = nullptr;
		freeSlots_ = nullptr;
		blockCount_ = 0;
		LOG_INFO << "MemoryPool block size:" << BlockSize << " element size:" << sizeof(T);
		allocateBlock();
	};
	MemoryPool(const MemoryPool& memoryPool) = delete;
	MemoryPool(MemoryPool&& memoryPool) = delete;
	template <class U> MemoryPool(const MemoryPool<U>& memoryPool) = delete;
	MemoryPool& operator=(const MemoryPool& memoryPool) = delete;
	MemoryPool& operator=(MemoryPool&& memoryPool) = delete;

	~MemoryPool()
	{
		Slot* curr = currentBlock_;
		while (curr != nullptr) 
		{
			Slot* prev = curr->next;
			operator delete(reinterpret_cast<void*>(curr));
			curr = prev;
		}
	};
	T* allocate()
	{
		if (freeSlots_ != nullptr)
		{
			T* result = reinterpret_cast<T*>(freeSlots_);
			freeSlots_ = freeSlots_->next;
			return result;
		}
		else if (currentSlot_ >= lastSlot_)
		{
			allocateBlock();
		}
		return reinterpret_cast<T*>(currentSlot_++);
	};
	void deallocate(T* p)
	{
		if (p != nullptr)
		{
			reinterpret_cast<Slot*>(p)->next = freeSlots_;
			freeSlots_ = reinterpret_cast<Slot*>(p);
		}
	};

	template <class... Args> T* newElement(Args&&... args)
	{
		T* result = allocate();
		new (result) T(std::forward<Args>(args)...);
		return result;
	};
	void deleteElement(T* p)
	{
		if (p != nullptr) 
		{
			p->~T();
			reinterpret_cast<Slot*>(p)->next = freeSlots_;
			freeSlots_ = reinterpret_cast<Slot*>(p);
		}
	};
	void setName(std::string name)
	{
		name_ = name;
	}
	std::string& getName()
	{
		return name_;
	}
private:

	size_t padPointer(char* p, size_t align) const
	{
		uintptr_t result = reinterpret_cast<uintptr_t>(p);
		return ((align - result) % align);
	};
	void allocateBlock()
	{
		// Allocate space for the new block and store a pointer to the previous one
		char* newBlock = reinterpret_cast<char*>(operator new(BlockSize));
		reinterpret_cast<Slot*>(newBlock)->next = currentBlock_;
		currentBlock_ = reinterpret_cast<Slot*>(newBlock);

		// Pad block body to staisfy the alignment requirements for elements
		char* body = newBlock + sizeof(Slot*);
		size_t bodyPadding = padPointer(body, alignof(Slot));
		
		currentSlot_ = reinterpret_cast<Slot*>(body + bodyPadding);
		lastSlot_ = reinterpret_cast<Slot*>(newBlock + BlockSize - sizeof(Slot) + 1);
		blockCount_++;
		LOG_INFO << name_ << " allocateBlock block count:" << blockCount_;
	};
	
private:
	Slot* currentBlock_;
	Slot* currentSlot_;
	Slot* lastSlot_;
	Slot* freeSlots_;
	std::string name_;
	int blockCount_;
};
#endif