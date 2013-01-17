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

//-----------------------------------------------------------------------
// Local Macros, Typedefs, Structures, Unions and Forward Declarations --
//-----------------------------------------------------------------------

namespace {

  const char* logger = "XtcChunkDgIter";

}


//		----------------------------------------
// 		-- Public Function Member Definitions --
//		----------------------------------------

namespace XtcInput {

//----------------
// Constructors --
//----------------
XtcChunkDgIter::XtcChunkDgIter (const std::string& path, unsigned liveTimeout)
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
  // move to a correct position
  if (m_file.seek(m_off, SEEK_SET) == (off_t)-1) {
    throw XTCReadException(ERR_LOC, m_file.path().string());
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
    throw XTCReadException(ERR_LOC, m_file.path().string());
  } else if (nread != ssize_t(headerSize)) {
    MsgLog(logger, error, "EOF while reading datagram header from file: " << m_file.path());
    return hptr;
  }

  WithMsgLog(logger, debug, str) {
    str << "header:";
    uint32_t* p = (uint32_t*)&header;
    for (int i = 0; i != 10; ++ i) str << ' ' << p[i];
  }

  // make an object
  hptr = boost::make_shared<DgHeader>(header, m_file, m_off);

  // get position of the next datagram
  m_off = hptr->nextOffset();

  return hptr;
}

} // namespace XtcInput
