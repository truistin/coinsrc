// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef SPOT_UTILITY_COUNTDOWNLATCH_H
#define SPOT_UTILITY_COUNTDOWNLATCH_H

#include <spot/utility/Condition.h>
#include <spot/utility/NonCopyAble.h>
#include <mutex>

namespace spot
{

class CountDownLatch : utility::NonCopyable
{
 public:

  explicit CountDownLatch(int count);

  void wait();

  void countDown();

  int getCount() const;

 private:
	mutable MutexLock mutex_;
  Condition condition_;
  int count_;
};

}
#endif  // MUDUO_BASE_COUNTDOWNLATCH_H
