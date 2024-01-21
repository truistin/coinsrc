// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef SPOT_UTILITY_THREAD_H
#define SPOT_UTILITY_THREAD_H

#include <functional>
#include <spot/utility/NonCopyAble.h>
#include <spot/utility/Compatible.h>
#include <spot/utility/Types.h>
#include <memory>
#include <thread>
#include <atomic>
namespace spot
{
class Thread : utility::NonCopyable
{
 public:
  typedef std::function<void ()> ThreadFunc;


  explicit Thread(ThreadFunc&&, const string& name);

	~Thread();

  void start();
  void join(); // return pthread_join()

  bool started() const { return started_; }
 
	PID_T tid() const { return *tid_; }
  const string& name() const { return name_; }

  static int numCreated() { return numCreated_; }
  std::shared_ptr<std::thread> pthreadId_;

 private:
  void setDefaultName();

  bool       started_;
  bool       joined_;
	std::shared_ptr<PID_T > tid_;
  ThreadFunc func_;
  string     name_;

	static std::atomic<int32_t> numCreated_;
};

}
#endif
