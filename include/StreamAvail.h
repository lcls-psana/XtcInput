#ifndef XTCINPUT_STREAMAVAIL_H
#define XTCINPUT_STREAMAVAIL_H

//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class StreamAvail
//
//------------------------------------------------------------------------

//-----------------
// C/C++ Headers --
//-----------------
#include "boost/shared_ptr.hpp"
#include <vector>
#include <map>

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "FileIO/FileIO_I.h"
#include "XtcInput/XtcFileName.h"
#include "XtcInput/L1AcceptsFollowing.h"
#include "XtcInput/CountUpcomingSorted.h"

//		---------------------
// 		-- Class Interface --
//		---------------------


namespace XtcInput {

  /**
   * Maintains maps of stream,chunk pairs to Event counters.
   * provides method "countUpTo" to get available events on disk.
   */
class StreamAvail {

 public:
  
  StreamAvail(boost::shared_ptr<FileIO::FileIO_I> fileIO);

  unsigned countUpTo(const XtcFileName &xtcFileName, off_t offset, unsigned maxToCount);

  ~StreamAvail();

 private:
  boost::shared_ptr<FileIO::FileIO_I> m_fileIO;

  typedef XtcInput::CountUpcomingSorted<off_t, L1AcceptOffsetsFollowingFunctor> ChunkCounter;
  typedef std::pair<unsigned, unsigned> StreamChunk;
  typedef std::map<StreamChunk, ChunkCounter> StreamChunk2CounterMap;
  StreamChunk2CounterMap m_streamChunk2counter;

  std::vector<int> m_openFileDescriptors;
};

} // XtcInput

#endif  
