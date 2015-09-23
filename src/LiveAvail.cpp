//--------------------------------------------------------------------------
// File and Version Information:
//     $Id: DgramSourceFile.cpp 9615 2015-02-12 17:21:37Z davidsch@SLAC.STANFORD.EDU $
//
// Description:
//     Class DgramSourceFile...
//
// Author List:
//     Andy Salnikov
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------
#include "XtcInput/LiveAvail.h"

//-----------------
// C/C++ Headers --
//-----------------

//-------------------------------
// Collaborating Class Headers --
//-------------------------------

//-----------------------------------------------------------------------
// Local Macros, Typedefs, Structures, Unions and Forward Declarations --
//-----------------------------------------------------------------------

using namespace XtcInput;

//             ----------------------------------------
//             -- Public Function Member Definitions --
//             ----------------------------------------

namespace {

}

namespace XtcInput {

//----------------
// Constructors --
//----------------
LiveAvail::LiveAvail(XtcMergeIterator *xtcMergerIter) :
  m_xtcMergeIter(xtcMergerIter) 
{ 
}


//--------------
// Destructor --
//--------------
  LiveAvail::~LiveAvail() 
{
}


bool LiveAvail::availEventsIsAtLeast(unsigned numEvents) {
  if (m_xtcMergeIter) {
    return m_xtcMergeIter->availEventsIsAtLeast(numEvents);
  } else {
    return false;
  }
}

void LiveAvail::mergerAboutToBeDestroyed() { 
  m_xtcMergeIter = NULL;
}


}; // namespace XtcInput
