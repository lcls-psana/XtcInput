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

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "MsgLogger/MsgLogger.h"

//-----------------------------------------------------------------------
// Local Macros, Typedefs, Structures, Unions and Forward Declarations --
//-----------------------------------------------------------------------

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
      return fileIO.open(notInprogress.c_str(), O_RDONLY | O_LARGEFILE);
    }
    return fid;
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
}

StreamAvail::~StreamAvail()
{
  for (unsigned idx = 0; idx < m_openFileDescriptors.size(); ++idx) {
    int res = m_fileIO->close(m_openFileDescriptors[idx]);
    MsgLog(logger, trace, "closed fd=" << m_openFileDescriptors[idx] << " return code=" << res);
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
  if (availThisChunk < maxToCount) {
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
      MsgLog(logger, trace, "At chunk boundary. Counting available for " 
             << xtcFileName << ", will also try " << nextXtc);
      m_openFileDescriptors.push_back(fid);
      L1AcceptOffsetsFollowingFunctor nextCounter(fid, m_fileIO);
      m_streamChunk2counter.insert(std::pair<StreamChunk, ChunkCounter>(streamNextChunk, ChunkCounter(nextCounter)));
    }
    ChunkCounter &nextCounter = m_streamChunk2counter.find(streamNextChunk)->second;
    return availThisChunk + nextCounter.afterUpTo(0, maxToCount - availThisChunk);
  }
  return availThisChunk;
}

//              ----------------------------------------
//              -- Private Function Member Definitions --
//              ----------------------------------------

}; // namespace XtcInput
