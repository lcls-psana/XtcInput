//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class XtcChunkDgIter...
//
// Author List:
//      Andrei Salnikov
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------
#include "XtcInput/XtcChunkDgIter.h"

//-----------------
// C/C++ Headers --
//-----------------
#include <boost/make_shared.hpp>

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "XtcInput/Exceptions.h"
#include "MsgLogger/MsgLogger.h"
#include "LusiTime/Time.h"

//-----------------------------------------------------------------------
// Local Macros, Typedefs, Structures, Unions and Forward Declarations --
//-----------------------------------------------------------------------

namespace {

  const char* logger = "XtcInput.XtcChunkDgIter";

}


//		----------------------------------------
// 		-- Public Function Member Definitions --
//		----------------------------------------

namespace XtcInput {

//----------------
// Constructors --
//----------------
XtcChunkDgIter::XtcChunkDgIter (const XtcFileName& path, unsigned liveTimeout)
  : m_file(path, liveTimeout)
  , m_off(0)
{
}

//--------------
// Destructor --
//--------------
XtcChunkDgIter::~XtcChunkDgIter ()
{
}

boost::shared_ptr<DgHeader>
XtcChunkDgIter::next()
{
  return nextAtOffset(m_off);
}

boost::shared_ptr<DgHeader>
XtcChunkDgIter::nextAtOffset(off64_t offset)
{
  if (m_file.seek(offset, SEEK_SET) == (off_t)-1) {
    throw XTCReadException(ERR_LOC, m_file.path().path());
  }

  boost::shared_ptr<DgHeader> hptr;

  // read header
  Pds::Dgram header;
  const size_t headerSize = sizeof header;
  MsgLog(logger, debug, "reading header");
  ssize_t nread = m_file.read(((char*)&header), headerSize);
  if (nread == 0) {
    // EOF
    return hptr;
  } else if (nread < 0) {
    throw XTCReadException(ERR_LOC, m_file.path().path());
  } else if (nread != ssize_t(headerSize)) {
    MsgLog(logger, error, "EOF while reading datagram header from file: " << m_file.path());
    return hptr;
  }

  WithMsgLog(logger, debug, str) {
    str << "header:";
    uint32_t* p = (uint32_t*)&header;
    for (int i = 0; i != 10; ++ i) str << ' ' << p[i];
  }

  // check header consistency
  if (header.xtc.extent < sizeof(Pds::Xtc)) {
    investigateCrash(header, offset);
  }

  // make an object
  hptr = boost::make_shared<DgHeader>(header, m_file, offset);

  // get position of the next datagram
  m_off = hptr->nextOffset();

  return hptr;
}

/// ----- hack code
void 
XtcChunkDgIter::reReadHeader(Pds::Dgram &header, off64_t offset) {
  Pds::Dgram reHeader;
  off64_t repositionedOffset = m_file.seek(offset, SEEK_SET);
  if (repositionedOffset != offset) {
    MsgLog(logger, error, "could not seek to offset: " 
           << offset << " got offset: " << repositionedOffset
           << " not going to re-read header");
    return;
  }
  const size_t headerSize = sizeof reHeader;
  MsgLog(logger, info, "reading header from reread-header");
  ssize_t nread = m_file.read(((char*)&reHeader), headerSize);
  if (nread == 0) {
    MsgLog(logger, error, "in rereadheader and read 0 bytes after repositioning to already read header. Returning.");
    return;
  } else if (nread < 0) {
    MsgLog(logger, error, "in rereadheader and read returned negative number for file error. Returning.");
    return;
  } else if (nread != ssize_t(headerSize)) {
    MsgLog(logger, error, "EOF while re-reading datagram header from file: " 
           << m_file.path() << " at offset " << offset << ". returning");
    return;
  }
  header = reHeader;
}


void XtcChunkDgIter::printDiagnosticMessage(Pds::Dgram &header, off64_t offset, int numTries)
{
  uint32_t *p = static_cast<uint32_t*>(static_cast<void *>(&header));
  LusiTime::Time timeNow = LusiTime::Time::now();
  WithMsgLog(logger, error, str) {
    str << "xtc.extent=" << header.xtc.extent 
        << " numTries=" << numTries 
        << " dgheader=";
    for (int i = 0; i < 10; ++i) {
      str << " 0x" << std::hex << p[i];
    }
    str << std::dec << " time_now=" << timeNow;
  }
}

void XtcChunkDgIter::investigateCrash(Pds::Dgram &header, off64_t offset) 
{
  int numTries = 1;
  int maxTries = 20;
  bool stillBad = true;
  struct stat buf;
  std::string fname = m_file.path().path();
  std::string inprogress, notinprogress;
  if ((fname.size()> 11) and (fname.substr(fname.size()-11) == ".inprogress")) {
    inprogress = fname;
    notinprogress = fname.substr(0,fname.size()-11);    
  } else {
    notinprogress = fname;
  }  

  MsgLog(logger, error, "=======Trying to recover from xtc.extent problem. Will make "
         << maxTries << " attempts to reread header=====");
  do {
    printDiagnosticMessage(header, offset, numTries);
    sleep(1);
    reReadHeader(header, offset);
    stillBad = header.xtc.extent  < sizeof(Pds::Xtc);
    numTries += 1;
    if (stillBad and (numTries > 5)) {
      int statResult = m_file.stat(&buf);
      MsgLog(logger, error, "   also tried to fstat() file based on file handle, fstat returned " << statResult);
    }
    if (stillBad and (numTries > 10)) {
      if (inprogress.size()>0) {
        int statRes = stat(inprogress.c_str(), &buf);
        MsgLog(logger, error, "   Tried stat() of inprogress file=" << inprogress << "  and got " << statRes);
      }
      if (notinprogress.size()>0) {
        int statRes = stat(notinprogress.c_str(), &buf);
        MsgLog(logger, error, "   Tried stat() of notinprogress file=" << notinprogress << "  and got " << statRes);
      }
    }
    numTries++;
  } while (stillBad and (numTries < maxTries));
  if (stillBad) {
    MsgLog(logger, error, "------xtcextent is still bad after 20 tries. Throwing exception.--------");
    throw XTCExtentException(ERR_LOC, m_file.path().path(), offset, header.xtc.extent);
  } 
}

} // namespace XtcInput
