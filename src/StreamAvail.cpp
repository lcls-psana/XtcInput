//--------------------------------------------------------------------------
// File and Version Information:
//     $Id$
//
// Description:
//     Class StreamAvail
//
// Author List:
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------
#include "XtcInput/StreamAvail.h"

//-----------------
// C/C++ Headers --
//-----------------
#include "boost/make_shared.hpp"

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "MsgLogger/MsgLogger.h"
#include "FileIO/StdFileIO.h"

//-----------------------------------------------------------------------
// Local Macros, Typedefs, Structures, Unions and Forward Declarations --
//-----------------------------------------------------------------------

#define TRACEMSG trace
#define DEBUGMSG debug

namespace {

  const char * logger = "StreamAvail";
  
  std::string  
  stripInProgress(const XtcInput::XtcFileName &xtcFileName) {
    std::string path = xtcFileName.path();
    if (xtcFileName.extension() == ".inprogress") {
      path = path.substr(0, path.size()-11);
    }
    return path;
  }

  int tryToOpen(const XtcInput::XtcFileName &xtcFname, FileIO::FileIO_I &fileIO) {
    std::string notInprogress = stripInProgress(xtcFname);
    std::string inProgress = notInprogress + ".inprogress";
    int fid = fileIO.open(inProgress.c_str(), O_RDONLY | O_LARGEFILE);
    if (fid < 0) {
      fid = fileIO.open(notInprogress.c_str(), O_RDONLY | O_LARGEFILE);
      if (fid >= 0) {
        MsgLog(logger, TRACEMSG, "Opened file descriptor " << fid << " for " << notInprogress);
      }
    } else {
      MsgLog(logger, TRACEMSG, "Opened file descriptor " << fid << " for " << inProgress);
    }
    return fid;
  }

  bool notInProgressExists(const XtcInput::XtcFileName &xtcFname, FileIO::FileIO_I &fileIO) {
    std::string notInprogress = stripInProgress(xtcFname);
    int fid = fileIO.open(notInprogress.c_str(), O_RDONLY | O_LARGEFILE);
    bool result = fid >= 0;
    if (result) fileIO.close(fid);
    return result;
  }
  std::string getDir(const XtcInput::XtcFileName & xtc) {
    size_t baseLen = xtc.basename().size();
    std::string res = xtc.path();
    std::string dir = res.erase(res.size()-baseLen);
    return dir;
  }

}

namespace XtcInput {

//              ----------------------------------------
//              -- Public Function Member Definitions --
//              ----------------------------------------
StreamAvail::StreamAvail(boost::shared_ptr<FileIO::FileIO_I> fileIO) :
  m_fileIO(fileIO)
{
  if (not m_fileIO) {
    m_fileIO = boost::make_shared<FileIO::StdFileIO>();
  }
}

StreamAvail::~StreamAvail()
{
  for (unsigned idx = 0; idx < m_openFileDescriptors.size(); ++idx) {
    int res = m_fileIO->close(m_openFileDescriptors[idx]);
    MsgLog(logger, TRACEMSG, "closed fd=" << m_openFileDescriptors[idx] << " return code=" << res);
  }
  m_openFileDescriptors.clear();
}

unsigned StreamAvail::countUpTo(const XtcFileName &xtcFileName, off_t offset, unsigned maxToCount)
{
  std::pair<unsigned, unsigned> streamChunk(xtcFileName.stream(), xtcFileName.chunk());

  if (m_streamChunk2counter.find(streamChunk) == m_streamChunk2counter.end()) {
    int fid = tryToOpen(xtcFileName, *m_fileIO);
    if (fid < 0) {
      MsgLog(logger, error, "Could not open file: " << xtcFileName << " that should exist");
      return 0;
    }
    m_openFileDescriptors.push_back(fid);
    L1AcceptOffsetsFollowingFunctor counter(fid, m_fileIO);
    m_streamChunk2counter.insert(std::pair<StreamChunk, ChunkCounter>(streamChunk, ChunkCounter(counter)));
  }
  ChunkCounter &counter = m_streamChunk2counter.find(streamChunk)->second;
  unsigned availThisChunk = counter.afterUpTo(offset, maxToCount);
  if ((availThisChunk < maxToCount) and notInProgressExists(xtcFileName, *m_fileIO)) {
    std::pair<unsigned, unsigned> streamNextChunk(xtcFileName.stream(), 1 + xtcFileName.chunk());
    if (m_streamChunk2counter.find(streamNextChunk) == m_streamChunk2counter.end()) {
      XtcFileName nextXtc = XtcFileName(getDir(xtcFileName),
                                        xtcFileName.expNum(),
                                        xtcFileName.run(),
                                        streamNextChunk.first,
                                        streamNextChunk.second, 
                                        xtcFileName.small());
      int fid = tryToOpen(nextXtc, *m_fileIO);
      if (fid < 0) return availThisChunk;
      MsgLog(logger, TRACEMSG, "At chunk boundary. Counting available for " 
             << xtcFileName << ", will also try " << nextXtc);
      m_openFileDescriptors.push_back(fid);
      L1AcceptOffsetsFollowingFunctor nextCounter(fid, m_fileIO);
      m_streamChunk2counter.insert(std::pair<StreamChunk, ChunkCounter>(streamNextChunk, ChunkCounter(nextCounter)));
    }
    ChunkCounter &nextCounter = m_streamChunk2counter.find(streamNextChunk)->second;
    return availThisChunk + nextCounter.afterUpTo(0, maxToCount - availThisChunk);
  }
  MsgLog(logger, DEBUGMSG, "availThisChunk=" << availThisChunk);
  return availThisChunk;
}

//              ----------------------------------------
//              -- Private Function Member Definitions --
//              ----------------------------------------

}; // namespace XtcInput
