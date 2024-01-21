// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef SPOT_UTILITY_BLOCKINGQUEUE_H
#define SPOT_UTILITY_BLOCKINGQUEUE_H

#include <spot/utility/Condition.h>
#include <spot/utility/NonCopyAble.h>
#include <spot/utility/Mutex.h>
#include <deque>
#include <functional>
#include <assert.h>

namespace spot
{

template<typename T>
class BlockingQueue : utility::NonCopyable
{
 public:
  BlockingQueue()
    : notEmpty_(mutex_),
      queue_()
      
  {
  }


  void put(T&& x)
  {
		MutexLockGuard lock(mutex_);
		queue_.push_back(std::move(x));
    notEmpty_.notify();
  }


  T take()
  {
		MutexLockGuard lock(mutex_);
    // always use a while-loop, due to spurious wakeup
    while (queue_.empty())
    {
      notEmpty_.wait();
    }
    assert(!queue_.empty());
    
		T front(std::move(queue_.front()));

    queue_.pop_front();
    return front;
  }
  bool tryTake(T& x)
  {
    MutexLockGuard lock(mutex_);
    if (queue_.empty())
    {
      return false;
    }
    x = std::move(queue_.front());
    queue_.pop_front();
    return true;
  }
  size_t size() const
  {
		MutexLockGuard lock(mutex_);
    return queue_.size();
  }

 private:
  mutable MutexLock mutex_;
  Condition         notEmpty_;
  std::deque<T>     queue_;
  int milliseconds_;
};

}

#endif  
