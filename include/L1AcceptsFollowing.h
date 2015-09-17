#ifndef XTCINPUT_L1ACCEPTSFOLLOWING_H
#define XTCINPUT_L1ACCEPTSFOLLOWING_H

//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	
//
//------------------------------------------------------------------------

//-----------------
// C/C++ Headers --
//-----------------
#include <vector>
#include <boost/shared_ptr.hpp>

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "FileIO/FileIO_I.h"

namespace XtcInput {
    
  /**
   * @brief count L1Accepts available on disk, up to max, for open file
   * 
   * @param fd file descriptor for open file
   * @param offset position of datagram in file to start at. This datagram is not counted.
   * @param maxToCount maximum number of L1Accepts (following start) to count
   * @param fileIO fileIO interface for working with fd
   *
   * @return list of offsets for L1Accept datagrams following offset in fd. Size <= maxToCount
   */
std::vector<off_t> L1AcceptOffsetsFollowing(int fd, off_t offset, int maxToCount, 
                                            FileIO::FileIO_I &fileIO);


  /**
   * functor to call L1AcceptsFollowing by caching fd and fileIO
   */
class L1AcceptOffsetsFollowingFunctor {

public:
  L1AcceptOffsetsFollowingFunctor(int fd, boost::shared_ptr<FileIO::FileIO_I> fileIO) :
    m_fd(fd), m_fileIO(fileIO) {};
  std::vector<off_t> operator()(off_t offset, int maxToCount) {
   return L1AcceptOffsetsFollowing(m_fd, offset, maxToCount, *m_fileIO);
  }

private:
  int m_fd;
  boost::shared_ptr<FileIO::FileIO_I> m_fileIO;
}; // functor class
    
}

#endif
