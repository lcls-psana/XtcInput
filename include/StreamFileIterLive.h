#ifndef XTCINPUT_STREAMFILEITERLIVE_H
#define XTCINPUT_STREAMFILEITERLIVE_H

//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class StreamFileIterLive.
//
//------------------------------------------------------------------------

//-----------------
// C/C++ Headers --
//-----------------
#include <set>
#include <boost/shared_ptr.hpp>

//----------------------
// Base Class Headers --
//----------------------
#include "XtcInput/StreamFileIterI.h"

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "XtcInput/LiveFilesDB.h"
#include "IData/Dataset.h"

//------------------------------------
// Collaborating Class Declarations --
//------------------------------------

//		---------------------
// 		-- Class Interface --
//		---------------------

namespace XtcInput {

/// @addtogroup XtcInput

/**
 *  @ingroup XtcInput
 *
 *  @brief Implementation of StreamFileIterI interface which works with live data.
 *
 *  This software was developed for the LCLS project.  If you use all or
 *  part of it, please give an appropriate acknowledgment.
 *
 *  @version $Id$
 *
 *  @author Andy Salnikov
 */

class StreamFileIterLive : public StreamFileIterI {
public:

  /**
   *  @brief Make iterator instance.
   *
   *  @param[in] expName    Experiment name
   *  @param[in] run       Run number
   *  @param[in] streamsFilter list of streams to process (empty means all associated with run)
   *  @param[in] liveTimeout Timeout in second to wait for live data
   *  @param[in] runLiveTimeOut Timeout in seconds to wait for a new run
   *  @param[in] filesdb   Database connection
   */
  StreamFileIterLive (const std::string& expName, unsigned run, const std::set<unsigned> &streamsFilter, unsigned liveTimeout, unsigned runLiveTimeOut,
      const boost::shared_ptr<LiveFilesDB>& filesdb) ;

  // Destructor
  virtual ~StreamFileIterLive () ;

  /**
   *  @brief Return chunk iterator for next stream.
   *
   *  Zero pointer is returned after last stream.
   */
  virtual boost::shared_ptr<ChunkFileIterI> next();

  /**
   *  @brief Return stream number for the set of files returned from last next() call.
   */
  virtual unsigned stream() const;

protected:

private:

  std::string m_expName;
  unsigned m_run;
  unsigned m_liveTimeout;
  unsigned m_runLiveTimeout;
  boost::shared_ptr<LiveFilesDB> m_filesdb;
  bool m_initialized;
  unsigned m_lastStream;
  std::set<unsigned> m_streamsFilter, m_streamsToProcess;
};

} // namespace XtcInput

#endif // XTCINPUT_STREAMFILEITERLIVE_H
