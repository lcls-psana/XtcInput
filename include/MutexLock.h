#ifndef XTCINPUT_MUTEXLOCK_H
#define XTCINPUT_MUTEXLOCK_H

#include "boost/thread/mutex.hpp"

namespace XtcInput {

class MutexLock {
 public:
 MutexLock(boost::mutex &mutex) : m_mutex(mutex) { 
    m_mutex.lock(); 
  }
  ~MutexLock() { m_mutex.unlock(); }
 private:
  boost::mutex &m_mutex;
}; // class MutexLock


}; // namespace XtcInput

#endif
