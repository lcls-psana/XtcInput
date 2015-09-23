#ifndef XTCINPUT_LIVEAVAIL_H
#define XTCINPUT_LIVEAVAIL_H

//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class LiveAvail
//
//------------------------------------------------------------------------

//-----------------
// C/C++ Headers --
//-----------------

//----------------------
// Base Class Headers --
//----------------------

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "XtcInput/XtcMergeIterator.h"

//------------------------------------
// Collaborating Class Declarations --
//------------------------------------

//		---------------------
// 		-- Class Interface --
//		---------------------

namespace XtcInput {

class LiveAvail {
 public:
  LiveAvail(XtcMergeIterator *xtcMergerIter);

  ~LiveAvail();

  bool availEventsIsAtLeast(unsigned numEvents);

  void mergerAboutToBeDestroyed();

 private:
  XtcMergeIterator *m_xtcMergeIter;
}; // class LiveAvail

}; // namespace XtcInput

#endif
