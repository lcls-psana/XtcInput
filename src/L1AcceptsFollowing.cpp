//--------------------------------------------------------------------------
// File and Version Information:
//     $Id$
//
// Description:
//
// Author List:
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------
#include "XtcInput/L1AcceptsFollowing.h"
#include "XtcInput/Exceptions.h"

//-----------------
// C/C++ Headers --
//-----------------

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "pdsdata/xtc/Dgram.hh"
#include "MsgLogger/MsgLogger.h"

//-----------------------------------------------------------------------
// Local Macros, Typedefs, Structures, Unions and Forward Declarations --
//-----------------------------------------------------------------------

namespace {

  const char* logger = "XtcInput.L1AcceptsFollowing";
  const uint32_t MAXDGRAMSIZE = 250*(1<<20);
} // local namespace

namespace XtcInput {

  std::vector<off_t> L1AcceptOffsetsFollowing(int fd, off_t offset, int maxToCount, 
                                              FileIO::FileIO_I &fileIO) {
  std::vector<off_t> result;
  Pds::Dgram dgHeader;
  off_t fsize = fileIO.filesize(fd);

  bool first = false;
  off_t nextOffset = offset;

  while ((nextOffset + off_t(sizeof(Pds::Dgram)) <= fsize) and \
         (int(result.size()) < maxToCount)) {
    if (nextOffset != fileIO.lseek(fd, nextOffset, SEEK_SET)) {
      throw XTCGenException(ERR_LOC, "Could not lseek after checking filesize");
    }
    if (sizeof(Pds::Dgram) != fileIO.read(fd, &dgHeader, sizeof(Pds::Dgram))) {
      throw XTCGenException(ERR_LOC, "Could not read expected Dgram Header after checking filesize");
    }
    uint32_t xtcExtent = dgHeader.xtc.extent;
    if ((xtcExtent > MAXDGRAMSIZE) or (xtcExtent < sizeof(Pds::Xtc))) {
      MsgLog(logger, error, "xtcExtent=" << xtcExtent 
             << " is invalid (to small or large). At offset=" << nextOffset);
      throw XTCSizeLimitException(ERR_LOC, "unknown", xtcExtent, MAXDGRAMSIZE);
    }
    off_t oldOffset = nextOffset;
    nextOffset += off_t(xtcExtent) + off_t(sizeof(Pds::Dgram) - sizeof(Pds::Xtc));
    if (first) {
      first = false;
    } else if ((nextOffset <= fsize) and \
               (dgHeader.seq.service() == Pds::TransitionId::L1Accept)) {
      result.push_back(oldOffset);
    }
    MsgLog(logger, debug, "prev=" << oldOffset << " next=" << nextOffset 
           << " fsize=" << fsize << " result.size()=" << result.size());
  }
  
  return result;
}

} // namespace XtcInput
