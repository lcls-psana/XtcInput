//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class SharedFile...
//
// Author List:
//      Andy Salnikov
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------
#include "XtcInput/SharedFile.h"

//-----------------
// C/C++ Headers --
//-----------------

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "MsgLogger/MsgLogger.h"
#include "XtcInput/Exceptions.h"

//-----------------------------------------------------------------------
// Local Macros, Typedefs, Structures, Unions and Forward Declarations --
//-----------------------------------------------------------------------

namespace {

#define MSGLOGLVL debug
  
  const char* logger = "XtcInput.SharedFile";
  
  off_t getFileLength(int fd) {
    struct stat buff;
    int result = ::fstat(fd, &buff);
    if (result != 0) {
      throw XtcInput::ErrnoException(ERR_LOC, "getFileLength", "fstat failed");
    }
    return buff.st_size;
  }

  off_t getFileOffset(int fd) {
    off_t fileOffset = ::lseek(fd, 0, SEEK_CUR);
    if (fileOffset == -1) {
      MsgLog(logger, error, "lseek call failed for file fd=" << fd);
      throw XtcInput::ErrnoException(ERR_LOC, "lseek", "I/O call failed during read on live data.");
    }
    return fileOffset;
  }
}

//		----------------------------------------
// 		-- Public Function Member Definitions --
//		----------------------------------------

namespace XtcInput {

//----------------
// Constructors --
//----------------
SharedFile::SharedFileImpl::SharedFileImpl (const XtcFileName& argPath,
    unsigned argLiveTimeout)
  : path(argPath)
  , liveTimeout(argLiveTimeout)
  , fd(-1)
  , lastFileLength(-1)
{
  fd = open(path.path().c_str(), O_RDONLY|O_LARGEFILE);
  if (fd < 0) {
    // try to open again after dropping inprogress extension
    if (liveTimeout > 0 and path.extension() == ".inprogress") {
      std::string chop = path.path();
      chop.erase(chop.size()-11);
      path = XtcFileName(chop);
      fd = open(path.path().c_str(), O_RDONLY|O_LARGEFILE);
    }
  }

  // timeout is only used when we read live data
  if (path.extension() != ".inprogress") liveTimeout = 0;

  if (fd < 0) {
    MsgLog( logger, error, "failed to open input XTC file: " << path );
    throw FileOpenException(ERR_LOC, path.path()) ;
  } else {
    lastFileLength = getFileLength(fd);
    MsgLog( logger, trace, "opened input XTC file: " << path << " fd=" << fd 
            << " initial size from fstat: " << lastFileLength);
  }
}

//--------------
// Destructor --
//--------------
SharedFile::SharedFileImpl::~SharedFileImpl()
{
  if (fd >= 0) close(fd);
}


// Read up to size bytes from a file, if EOF is hit
// then check that it is real EOF or wait (in live mode only)
// Returns number of bytes read or negative number for errors
ssize_t
SharedFile::read(char* buf, size_t size)
{
  if (not m_impl->liveTimeout) {
    ssize_t nread = 0;
    bool interrupted = false;
    do {
      nread = ::read(m_impl->fd, buf, size);
      if (nread < 0) {
        interrupted = (errno == EINTR);
      }
    } while (interrupted);
    MsgLog(logger, debug, "read " << nread << " bytes");    
    return nread;
  }
    
  // live data
  off_t readFromOffset = getFileOffset(m_impl->fd);
  std::time_t t0 = std::time(0);
  std::time_t now = t0;
  off_t left = off_t(size);

  MsgLog(logger, MSGLOGLVL, "read size=" << size << " from " << m_impl->path
         << " current file pos=" << readFromOffset 
         << " len=" << m_impl->lastFileLength);

  while (left > 0) {
    bool readFromOffsetIsEOF = false;
    while ((m_impl->lastFileLength <= readFromOffset) and 
           ((now-t0) < m_impl->liveTimeout) and (not readFromOffsetIsEOF)) {
      sleep(1);
      now = std::time(0);
      m_impl->lastFileLength = getFileLength(m_impl->fd);
      if (m_impl->lastFileLength <= readFromOffset) {
        readFromOffsetIsEOF = this->eof();
      }

      MsgLog(logger, MSGLOGLVL, "slept 1 sec. New filelength= " 
             << m_impl->lastFileLength << " sec until timeout: " 
             << m_impl->liveTimeout - (now-t0)
             << " eof=" << readFromOffsetIsEOF);
    }

    if (m_impl->lastFileLength > readFromOffset) {
      off_t bytesNextRead = std::min(off_t(left), (m_impl->lastFileLength) - readFromOffset);
      ssize_t nread = ::read(m_impl->fd, buf+(size-left), size_t(bytesNextRead));
      if ((nread >= 0) and (nread != bytesNextRead)) {
        MsgLog(logger, error, "system read from file " << m_impl->path 
               << " returned only " << nread << " even though it appears that " 
               << bytesNextRead << " bytes are available by looking at file length");
      }
      if (nread > 0) {
        MsgLog(logger, MSGLOGLVL, "read " << nread << " bytes");
        // got some data
        readFromOffset += nread;
        left -= nread;
        // reset timeout
        t0 = std::time(0);
        continue;
      } else if (nread == 0) {
        // unexpected, we have already printed error message.
        // We'll continue the loop, but not reset the timer.
        continue;
      } else if (nread < 0) {
        // error, retry for interrupted reads. Don't reset counter.
        if (errno == EINTR) continue;
        // return negative value so client can error out. Note - we may have read some of
        // the buffer in, really should reset file pointer to position when function first
        // entered
        MsgLog(logger, MSGLOGLVL, "nread=" << nread << " returning");
        return nread; 
      }
    } else {
      if (readFromOffsetIsEOF or (this->eof())) {
        MsgLog(logger, MSGLOGLVL, "Live EOF detected");
        break;
      }
      MsgLog(logger, error, "Timed out while waiting for data in live mode for file: " << m_impl->path);
      throw XTCLiveTimeout(ERR_LOC, m_impl->path.path(), m_impl->liveTimeout);
    }
  }
  return size-left;
}

// check that we reached EOF while reading live data
bool
SharedFile::eof()
{
  // we are at EOF only when the file has been renamed to its final
  // name, but is still the same file (same inode) and it's size is
  // exactly the same as the current offset.

  // strip file extension
  std::string path = m_impl->path.path();
  std::string::size_type p = path.rfind('.');
  if (p == std::string::npos) {
    // file does not have an extension, can't tell anything, just say we
    // are not at the EOF and loop above will timeout eventually
    return false;
  }
  const std::string pathFinal(path, 0, p);

  // check final file, get its info
  struct stat statFinal;
  if (::stat(pathFinal.c_str(), &statFinal) < 0) {
    // no such file, means no EOF yet
    return false;
  }

  // check current position
  off_t offset = ::lseek(m_impl->fd, 0, SEEK_CUR);
  if (offset == (off_t)-1) {
    MsgLog(logger, error, "error returned from lseek: " << errno << " -- " << strerror(errno));
    return false;
  }
  if (offset != statFinal.st_size) {
    // final has size different from our current position
    return false;
  }

  // info for current file
  struct stat statCurrent;
  if (::fstat(m_impl->fd, &statCurrent) < 0) {
    MsgLog(logger, error, "error returned from stat: " << errno << " -- " << strerror(errno));
    return false;
  }

  // compare inodes
  return statFinal.st_dev == statCurrent.st_dev and statFinal.st_ino == statCurrent.st_ino;
}

} // namespace XtcInput
