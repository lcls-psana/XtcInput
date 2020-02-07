#ifndef XTCINPUT_RUNFILEITERLIVE_H
#define XTCINPUT_RUNFILEITERLIVE_H

//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class RunFileIterLive.
//
//------------------------------------------------------------------------

//-----------------
// C/C++ Headers --
//-----------------
#include <set>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

//----------------------
// Base Class Headers --
//----------------------
#include "XtcInput/RunFileIterI.h"

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "XtcInput/LiveFilesWS.h"
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
 *  @brief Implementation of RunFileIterI interface working with live data.
 *
 *  This software was developed for the LCLS project.  If you use all or
 *  part of it, please give an appropriate acknowledgment.
 *
 *  @version $Id$
 *
 *  @author Andy Salnikov
 */

class RunFileIterLive : public RunFileIterI {
public:

  /**
   *  @brief Make iterator instance.
   *
   *  Constructor takes sequence of run numbers in the form of iterators.
   *
   *  @param[in] begin     Iterator pointing to the beginning of run number sequence
   *  @param[in] end       Iterator pointing to the end of run number sequence
   *  @param[in] expName    Experiment name
   *  @param[in] streamsFilter A ranges of streams (empty means all streams)
   *  @param[in] liveTimeout Specifies timeout in second when reading live data
   *  @param[in] runLiveTimeout Specifies timeout in second when waiting for a new run to show in the database when reading live data
   *  @param[in] wsConnStr Web service connection string
   *  @param[in] dir       Directory to look for live files
   *  @param[in] small     look for small data files
   */
  template <typename Iter>
    RunFileIterLive (Iter begin, Iter end, const std::string& expName,
                     const std::set<unsigned> &streamsFilter,
                     unsigned liveTimeout, unsigned runLiveTimeout,
                     const std::string& wsConnStr,
                     const std::string& dir, bool small)
    : RunFileIterI()
    , m_runs(begin, end)
    , m_expName(expName)
    , m_streamsFilter(streamsFilter)
    , m_liveTimeout(liveTimeout)
    , m_runLiveTimeout(runLiveTimeout)
    , m_run(0)
    , m_filesdb(boost::make_shared<LiveFilesWS>(wsConnStr, dir, small))
  {
  }

  // Destructor
  virtual ~RunFileIterLive () ;

  /**
   *  @brief Return stream iterator for next run.
   *
   *  Zero pointer is returned after last run.
   */
  virtual boost::shared_ptr<StreamFileIterI> next();

  /**
   *  @brief Return run number for the set of files returned from last next() call.
   */
  virtual unsigned run() const;

protected:

private:

  std::set<unsigned> m_runs;
  std::string m_expName;
  std::set<unsigned> m_streamsFilter;
  unsigned m_liveTimeout;
  unsigned m_runLiveTimeout;
  unsigned m_run;
  boost::shared_ptr<LiveFilesWS> m_filesdb;
};

} // namespace XtcInput

#endif // XTCINPUT_RUNFILEITERLIVE_H
