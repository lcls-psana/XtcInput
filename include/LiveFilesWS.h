#ifndef XTCINPUT_LIVEFILESWS_H
#define XTCINPUT_LIVEFILESWS_H

//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class LiveFilesWS.
//
//------------------------------------------------------------------------

//-----------------
// C/C++ Headers --
//-----------------
#include <string>
#include <vector>
#include <boost/utility.hpp>

//----------------------
// Base Class Headers --
//----------------------

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "XtcInput/XtcFileName.h"

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
 *  @brief Class which implements interface to migration database
 *
 *  This software was developed for the LCLS project.  If you use all or
 *  part of it, please give an appropriate acknowledgment.
 *
 *  @version $Id$
 *
 *  @author Andy Salnikov
 */

class LiveFilesWS : boost::noncopyable {
public:

  /**
   *  @brief Make an instance
   *
   *  @param[in] wsURL    The web service endpoint.
   *  @param[in] dir      Directory to look for live large xtc files
   *  @param[in] small    return filepaths for small xtc files: add 'smalldata' to dir, and use suffix .smd.xtc
   */
  LiveFilesWS(const std::string& wsURL, const std::string& dir, bool small);

  // Destructor
  ~LiveFilesWS () ;

  /**
   *  @brief Returns the list of files for given run. Constructor arguments dir and small affect paths returned
   *
   *  @param[in] expId    Experiment id
   *  @param[in] run      Run number
   */
  std::vector<XtcFileName> files(const std::string& expName, unsigned run);

protected:

private:

  std::string m_wsURL;          ///< The web service endpoint URL
  std::string m_dir;          ///< Directory to look for live large files
  bool m_small;               ///< return filepaths for small xtc files: add 'smalldata' to dir, and use suffix .smd.xtc

};

} // namespace XtcInput

#endif // XTCINPUT_LIVEFILESWS_H
