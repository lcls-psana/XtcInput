#ifndef XTCINPUT_COUNTUPCOMINGSORTED
#define XTCINPUT_COUNTUPCOMINGSORTED

//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//
//------------------------------------------------------------------------

//---------------
// C++ Headers --
//---------------
#include <deque>
#include <algorithm>

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "XtcInput/Exceptions.h"

namespace XtcInput {

  /**
   * @brief efficiently implements repeated counts of sorted things by caching.
   *
   * Use this to repeatedly count things that are expensive to identify - i.e, 
   * the upcoming datagrams in a file, where you want to reduce disk access.
   *
   * Takes a functor that will return a list of things after a starting
   * point (list is assumed to be sorted). The functor would implement disk
   * I/O for a list of datagrams. An instance of this class will use
   * the functor to get the upcoming things. It caches these things so as to
   * minimize calls to the functor.
   */
  template <class T, class Functor>
  class CountUpcomingSorted {
  public:
    CountUpcomingSorted(Functor &functor) : m_functor(functor) {};

    unsigned afterUpTo(T startAfterThis, unsigned maxToCount) {
      while ((m_cache.size()>0) and                 \
             (m_cache.front() < startAfterThis)) {
        m_cache.pop_front();
      }
      if ((m_cache.size()>0) and (m_cache.front() != startAfterThis)) {
        m_cache.clear();
      }
      unsigned amountInCache = m_cache.size();
      if ((amountInCache > 0) and (m_cache.size()>0) and (m_cache.front() == startAfterThis)) {
        amountInCache -= 1;
      }
      if (amountInCache >= maxToCount) return maxToCount;

      T functorStart = startAfterThis;
      if (amountInCache > 0) {
        functorStart = m_cache.back();
      }

      std::vector<T> newValues = m_functor(functorStart, maxToCount - amountInCache);

      bool isSorted = true;
      for (unsigned pos = 1; pos < newValues.size(); ++pos) {
        if (newValues.at(pos) < newValues.at(pos-1)) {
          isSorted = false;
          break;
        }
      }
        
      if (not isSorted) {
        throw NotSorted(ERR_LOC);
      }

      m_cache.insert(m_cache.end(), newValues.begin(), newValues.end());

      return newValues.size() + amountInCache;
    }

  private:
    Functor m_functor;
    typedef std::deque<T> DequeList;
    DequeList m_cache;
  };

} // namespace XtcInput

#endif
