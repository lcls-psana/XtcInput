//--------------------------------------------------------------------------
// File and Version Information:
//     $Id$
//
// Description:
//     Class XtcStreamDgIter...
//
// Author List:
//      Andrei Salnikov
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------
#include "XtcInput/XtcStreamDgIter.h"

//-----------------
// C/C++ Headers --
//-----------------
#include <boost/make_shared.hpp>

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "MsgLogger/MsgLogger.h"
#include "XtcInput/Exceptions.h"
#include "XtcInput/XtcChunkDgIter.h"
#include "pdsdata/xtc/Xtc.hh"
#include "XtcInput/Exceptions.h"

//-----------------------------------------------------------------------
// Local Macros, Typedefs, Structures, Unions and Forward Declarations --
//-----------------------------------------------------------------------

namespace {

  const char* logger = "XtcInput.XtcStreamDgIter" ;

  // size of the read-ahead buffer. The size of the buffer is the range
  // over which we can repair split events and reorder datagrams in time order.
  // Making it too long delays live mode. Roughly want a seconds worth of
  // data per stream. For an experiment with 6 DAQ streams at 120hz, this
  // would be 20 for a DAQ stream and 120 for a control stream, however
  // a control stream need not be recording at 120hz, and we don't expect
  // to have to look more than a few datagrams into the past to repair
  // control stream. 
  const unsigned daqReadAheadSize = 20;
  const unsigned controlReadAheadSize = 40;

  // functor to match header against specified clock time
  struct MatchClock {
    MatchClock(const Pds::ClockTime& clock) : m_clock(clock) {}
    bool operator()(const boost::shared_ptr<XtcInput::DgHeader>& header) const {
      return header->clock() == m_clock;
    }

    Pds::ClockTime m_clock;
  };

  // function to compare xtc files
  bool xtcFilesNotEqual(const XtcInput::XtcFileName &a, const XtcInput::XtcFileName &b) {
    int chunkA = a.chunk();
    int chunkB = b.chunk();
    bool equalChunks = chunkA == chunkB;
    bool notEqualPaths  = false;
    if (a < b) {
      notEqualPaths = true;
    }
    if (b < a) {
      notEqualPaths = true;
    }
    if (equalChunks and notEqualPaths) {
      MsgLog(logger,warning, "xtc files are not equal but chunks are: a=" << a << " b=" << b);
    }
    return notEqualPaths;
  }
}

//             ----------------------------------------
//             -- Public Function Member Definitions --
//             ----------------------------------------

namespace XtcInput {

//----------------
// Constructors --
//----------------
XtcStreamDgIter::XtcStreamDgIter(const boost::shared_ptr<ChunkFileIterI>& chunkIter,
                                 bool controlStream)
  : m_chunkIter(chunkIter)
  , m_dgiter()
  , m_chunkCount(0)
  , m_streamCount(0)
  , m_headerQueue()
  , m_controlStream(controlStream)
{
  if (controlStream) {
    m_headerQueue.reserve(::controlReadAheadSize);
  } else {
    m_headerQueue.reserve(::daqReadAheadSize);
  }
}

XtcStreamDgIter::XtcStreamDgIter(const boost::shared_ptr<ChunkFileIterI>& chunkIter,
                                 const boost::shared_ptr<ThirdDatagram> & thirdDatagram,
                                 bool controlStream)
  : m_chunkIter(chunkIter)
  , m_dgiter()
  , m_chunkCount(0)
  , m_streamCount(0)
  , m_headerQueue()
  , m_controlStream(controlStream)
  , m_thirdDatagram(thirdDatagram)
    
{
  if (controlStream) {
    m_headerQueue.reserve(::controlReadAheadSize);
  } else {
    m_headerQueue.reserve(::daqReadAheadSize);
  }
}
//--------------
// Destructor --
//--------------
XtcStreamDgIter::~XtcStreamDgIter ()
{
}

// read next datagram, return zero pointer after last file has been read,
// throws exception for errors.
Dgram
XtcStreamDgIter::next()
{
  // call other method to fill up and sort the queue
  readAhead();

  // pop one datagram if queue is not empty
  Dgram dgram;
  while (not m_headerQueue.empty()) {
    boost::shared_ptr<DgHeader> hptr = m_headerQueue.front();
    m_headerQueue.erase(m_headerQueue.begin());
    Dgram::ptr dg = hptr->dgram();
    if (dg) {
      dgram = Dgram(dg, hptr->path(), hptr->offset());
      break;
    } else {
      // header failed to read datagram, this is likely due to non-fatal
      // error like premature EOF. Skip this one and try to go to the next
      readAhead();
    }
  }

  return dgram ;
}

// fill the read-ahead queue
void
XtcStreamDgIter::readAhead()
{
  unsigned readAheadSize;
  if (m_controlStream) {
    readAheadSize = ::controlReadAheadSize;
  } else {
    readAheadSize = ::daqReadAheadSize;
  }
  
  while (m_headerQueue.size() < readAheadSize) {

    if (not m_dgiter) {

      // get next file name
      const XtcFileName& file = m_chunkIter->next();

      // if no more file then stop
      if (file.path().empty()) break ;

      // open next xtc file if there is none open
      MsgLog(logger, trace, "processing file: " << file) ;
      m_dgiter = boost::make_shared<XtcChunkDgIter>(file, m_chunkIter->liveTimeout());
      m_chunkCount = 0 ;
    }

    boost::shared_ptr<DgHeader> hptr; // next datagram header

    // check for special parameters to jump for the third datagram 
    if (m_streamCount == 2) {    // we are at the third datagram, streamCount is 0 based
      if (m_thirdDatagram) {
        const XtcFileName & xtcFileForThirdDgram = m_thirdDatagram->xtcFile;
        off64_t offsetForThirdDgram = m_thirdDatagram->offset;
        MsgLog(logger,debug,"third datagram jump: will try to jump to offset=" 
	       << offsetForThirdDgram << " in file=" << xtcFileForThirdDgram); 
        // advance chunk file if need be
        while (xtcFilesNotEqual(m_dgiter->path(), xtcFileForThirdDgram)) {
          MsgLog(logger,debug,"third datagram jump, jmpFile != currentFile - "
                 << xtcFileForThirdDgram << " != " << m_dgiter->path());
          // get next file name
          const XtcFileName& file = m_chunkIter->next();
          
          if (file.path().empty()) {
	    // we went through all the files in the chunkIter
            throw FileNotInStream(ERR_LOC, xtcFileForThirdDgram.path());
          }
          // open file
          MsgLog(logger, trace, " looking for third dgram - opening file: " << file) ;
          m_dgiter = boost::make_shared<XtcChunkDgIter>(file, m_chunkIter->liveTimeout());
          m_chunkCount = 0;
        }
        hptr = m_dgiter->nextAtOffset(offsetForThirdDgram);
      } else {
        // this is the third datagram (streamCount == 2), but no special jump to do
        MsgLog(logger,debug,"third datagram, no jmp");
        hptr = m_dgiter->next();
      }
    } else {
      // typical case, streamCount != 1
      hptr = m_dgiter->next();
    }

    // if failed to read go to next file
    if (not hptr) {
      m_dgiter.reset();
    } else {
      // read full datagram
      queueHeader(hptr);
      ++ m_chunkCount ;
      ++ m_streamCount ;
    }

  }

  MsgLog(logger, debug, "headers queue has size " << m_headerQueue.size()) ;

}

// add one header to the queue
void
XtcStreamDgIter::queueHeader(const boost::shared_ptr<DgHeader>& header)
{
  Pds::TransitionId::Value tran = header->transition();
  const Pds::ClockTime& clock = header->clock();
  MsgLog(logger, debug, "XtcStreamDgIter::queueHeader: transition: " << Pds::TransitionId::name(tran)
         << " sec: " << clock.seconds() << "nsec:" << clock.nanoseconds() << " fid: "
         << header->fiducials() << " controlStream=" << m_controlStream);

  // For split transitions look at the queue and find matching split transition,
  // store them together if found, otherwise assume it's first piece and store
  // it like normal transition. Match based on the clock.
  if (header->damage().value() & (1 << Pds::Damage::DroppedContribution)) {
    HeaderQueue::iterator it = std::find_if(m_headerQueue.begin(), m_headerQueue.end(), MatchClock(clock));
    if (it != m_headerQueue.end()) {
      MsgLog(logger, debug, "XtcStreamDgIter::queueHeader: split transition, found match");
      m_headerQueue.insert(it, header);
      return;
    }
  }
  

  // At this point we have a non-split transition or a split transition without
  // other pieces of the same split transitions (other pieces may have already appeared
  // in the stream, but have been popped from the queue). Below, we treat 
  // a single piece of a split transition the same as a non-split transition.

  // We keep the L1Accept datagrams in the queue sorted by clock time. We do 
  // not move a L1Accept past a non L1Accept transition even if sorting by clock time
  // indicates we should. Control streams use two clocks, an internal one for L1Accepts,
  // and the DAQ one for non transitions, so this would definitely not be correct for
  // control streams. 
  //
  // this sorting serves two purposes: to make sure each stream merger sees the earliest
  // known datagrams from each stream so that it can properly merge datagrams from the streams
  // into an event, and to time order events for the user.

  if (tran != Pds::TransitionId::L1Accept) {
    MsgLog(logger, debug, "XtcStreamDgIter::queueHeader: non-event transition, append");
    m_headerQueue.push_back(header);
    return;
  }

  /*
   *  At this point we have L1Accept. Start from the end of the 
   *  queue and walk to the head until we meet either earlier L1Accept or 
   * non-L1Accept transition. If we meet the same transition and this is
   * control stream, throw out datagram in the queue, and the new datagram.
   */
  for (HeaderQueue::iterator it = m_headerQueue.end(); it != m_headerQueue.begin(); -- it) {
    const boost::shared_ptr<DgHeader>& prev = *(it - 1);
    
    if (prev->transition() != Pds::TransitionId::L1Accept) {
      MsgLog(logger, debug, "XtcStreamDgIter::queueHeader: insert L1Accept after non-L1Accept");
      m_headerQueue.insert(it, header);
      return;
    } else if (clock > prev->clock()) {
      MsgLog(logger, debug, "XtcStreamDgIter::queueHeader: insert L1Accept after earlier L1Accept");
      m_headerQueue.insert(it, header);
      return;
    } else if (clock == prev->clock()) {
      if (m_controlStream) {
        MsgLog(logger, warning, "control stream has two datagrams with "
               "the same seconds/nanoseconds timestamp. "
               "The latter one is not marked as a split event. "
               "REMOVING the existing one, and Discarding the latter one: path=" << header->path()
               << " offset=" << header->offset() 
               << " sec=" << clock.seconds()
               << " nano=" << clock.nanoseconds()
               << " fiducials=" << header->fiducials()
               << " transition=" << Pds::TransitionId::name(header->transition()));
        m_headerQueue.erase(it);
        return;
      } else {
        MsgLog(logger, warning, "DAQ stream has two datagrams with "
               "the same seconds/nanoseconds timestamp. "
               "The latter one is not marked as a split event, it is being added to the queue. "
               "path=" << header->path()
               << " offset=" << header->offset() 
               << " sec=" << clock.seconds()
               << " nano=" << clock.nanoseconds()
               << " fiducials=" << header->fiducials()
               << " transition=" << Pds::TransitionId::name(header->transition()));
        m_headerQueue.insert(it, header);
        return;
      }
    }
  }
  // could not find any acceptable place, means this transition is earlier than all
  // other transitions, add it to the head of the queue
  MsgLog(logger, debug, "XtcStreamDgIter::queueHeader: insert L1Accept at the queue head");
  m_headerQueue.insert(m_headerQueue.begin(), header);
    
}

boost::shared_ptr<DgHeader> XtcStreamDgIter::latestDgHeaderInQueue() {
  if (m_headerQueue.size()==0) {
    return boost::shared_ptr<DgHeader>();
  }
  boost::shared_ptr<DgHeader> latest = m_headerQueue.front();
  // DVD REMOVE
  //  MsgLog(logger, info, "XtcStreamDgIter::latestDgHeaderInQueue front path=" << latest->path() << " offset=" << latest->offset());
  int latestChunk = latest->path().chunk();
  for (HeaderQueue::iterator it = m_headerQueue.begin(); it != m_headerQueue.end(); ++it) {
    int thisChunk = (*it)->path().chunk();
    if (thisChunk > latestChunk) {
      latest = *it;
    } else if (thisChunk == latestChunk) {
      if ((*it)->offset() > latest->offset()) {
        latest = *it;
      }
    }
  }
  // DVD REMOVE
  //  MsgLog(logger, info, "XtcStreamDgIter::latestDgHeaderInQueue latest path=" << latest->path() << " offset=" << latest->offset());
  return latest;
}
  
} // namespace XtcInput

