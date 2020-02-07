//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class StreamFileIterLive...
//
// Author List:
//      Andy Salnikov
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------
#include "XtcInput/StreamFileIterLive.h"

//-----------------
// C/C++ Headers --
//-----------------
#include <unistd.h>
#include <boost/make_shared.hpp>
#include <boost/lexical_cast.hpp>

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "MsgLogger/MsgLogger.h"
#include "XtcInput/ChunkFileIterLive.h"
#include "XtcInput/Exceptions.h"

//-----------------------------------------------------------------------
// Local Macros, Typedefs, Structures, Unions and Forward Declarations --
//-----------------------------------------------------------------------

namespace {

  const char* logger = "XtcInput.StreamFileIterLive";

}

//		----------------------------------------
// 		-- Public Function Member Definitions --
//		----------------------------------------

namespace XtcInput {

//----------------
// Constructors --
//----------------
  StreamFileIterLive::StreamFileIterLive (const std::string& expName, unsigned run, const std::set<unsigned> & streamsFilter,
                                        unsigned liveTimeout, unsigned runLiveTimeout, const boost::shared_ptr<LiveFilesWS>& filesdb)
  : StreamFileIterI()
  , m_expName(expName)
  , m_run(run)
  , m_liveTimeout(liveTimeout)
  , m_runLiveTimeout(runLiveTimeout)
  , m_filesdb(filesdb)
  , m_initialized(false)
  , m_lastStream(-1)
  , m_streamsFilter(streamsFilter)
{
}

//--------------
// Destructor --
//--------------
StreamFileIterLive::~StreamFileIterLive ()
{
}

/**
 *  @brief Return chunk iterator for next stream.
 *
 *  Zero pointer is returned after last stream.
 */
boost::shared_ptr<ChunkFileIterI>
StreamFileIterLive::next()
{
  boost::shared_ptr<ChunkFileIterI> next;

  if (not m_initialized) {

    // first time around get the list of streams from database

    m_initialized = true;

    std::vector<XtcFileName> files = m_filesdb->files(m_expName, m_run);
    if (files.empty() and (m_runLiveTimeout>0)) {
      MsgLog(logger, info, "database has no entry for run=" << m_run
             << " will wait for up to " << m_runLiveTimeout << " seconds.");
      std::time_t t0 = std::time(0);
      do {
        sleep(1);
        files = m_filesdb->files(m_expName, m_run);
      } while (files.empty() and std::time(0) < t0 + m_runLiveTimeout);
    }

    if (files.empty()) {
      throw ErrDbRunLiveData(ERR_LOC, m_run);
    }

    // wait for some time until at least one file appears on disk
    std::time_t t0 = std::time(0);
    bool found = false;
    while (not found) {
      for (std::vector<XtcFileName>::const_iterator it = files.begin(); it != files.end(); ++ it) {

        const std::string path = it->path();
        const std::string inprog_path = path + ".inprogress";

        if (access(inprog_path.c_str(), R_OK) == 0) {
          MsgLog(logger, debug, "Found file on disk: " << inprog_path);
          found = true;
          break;
        } else if (access(path.c_str(), R_OK) == 0) {
          MsgLog(logger, debug, "Found file on disk: " << path);
          found = true;
          break;
        }
      }
      if (std::time(0) > t0 + m_liveTimeout) break;
      // sleep for one second and repeat
      if (not found) {
        MsgLog(logger, debug, "Wait 1 sec for files to appear on disk");
        sleep(1);
      }
    }

    if (found) {
      // insertion of chunk 0 files (the streams) associated with a run into the database
      // may not be atomic. Now that a file has appeared on disk, update our list.
      // Assumption is that complete list of streams is available one a file shows up on disk.
      std::vector<XtcFileName> files = m_filesdb->files(m_expName, m_run);

      // copy stream numbers from the list
      for (std::vector<XtcFileName>::const_iterator it = files.begin(); it != files.end(); ++ it) {
        if ((m_streamsFilter.size()==0) or (m_streamsFilter.find(it->stream()) != m_streamsFilter.end())) {
          MsgLog(logger, debug, "Found stream to process: " << it->stream());
          m_streamsToProcess.insert(it->stream());
        } else {
          MsgLog(logger, debug, "Found stream " << it->stream() << " but it is filtered out. Not processing");
        }
      }
    } else {
      MsgLog(logger, error, "No files appeared on disk after timeout");
      throw XTCLiveTimeout(ERR_LOC, files[0].path(), m_liveTimeout);
    }

  } // if (not m_initialized)


  if (not m_streamsToProcess.empty()) {
    std::set<unsigned>::iterator s = m_streamsToProcess.begin();
    m_lastStream = *s;
    next = boost::make_shared<ChunkFileIterLive>(m_expName, m_run, m_lastStream, m_liveTimeout, m_filesdb);
    m_streamsToProcess.erase(s);
  }

  return next;
}

/**
 *  @brief Return stream number for the set of files returned from last next() call, or -1 if next() not called yet.
 */
unsigned
StreamFileIterLive::stream() const
{
  return m_lastStream;
}

} // namespace XtcInput
