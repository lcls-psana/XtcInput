//--------------------------------------------------------------------------
// File and Version Information:
//     $Id$
//
// Description:
//     Class DgramReader...
//
// Author List:
//     Andrei Salnikov
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------
#include "XtcInput/DgramReader.h"

//-----------------
// C/C++ Headers --
//-----------------
#include <algorithm>
#include <iterator>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/format.hpp>
#include <boost/make_shared.hpp>
#include <boost/thread/thread.hpp>

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "IData/Dataset.h"
#include "MsgLogger/MsgLogger.h"
#include "XtcInput/Exceptions.h"
#include "XtcInput/DgramQueue.h"
#include "XtcInput/RunFileIterList.h"
#include "XtcInput/RunFileIterLive.h"
#include "XtcInput/XtcFileName.h"
#include "XtcInput/XtcMergeIterator.h"
#include "pdsdata/xtc/Dgram.hh"

//-----------------------------------------------------------------------
// Local Macros, Typedefs, Structures, Unions and Forward Declarations --
//-----------------------------------------------------------------------

namespace fs = boost::filesystem;

namespace {

  const char* logger = "XtcInput.DgramReader";

  void splitIntoXtcFilesAndDatasets(const XtcInput::DgramReader::FileList &fileList, 
                                    std::vector<XtcInput::XtcFileName> &files, 
                                    std::vector<std::string> &datasets) {

    files.clear();
    datasets.clear();
    for (XtcInput::DgramReader::FileList::const_iterator it = fileList.begin(); 
         it != fileList.end(); ++ it) {
      IData::Dataset ds(*it);
      if (ds.isFile()) {
        files.push_back(XtcInput::XtcFileName(*it));
      } else {
        datasets.push_back(*it);
      }
    }
  }

  // When this returns, either:
  //  filenames.size()==0 and datasets.size()==1  
  //   or
  // filenames.size()>0 and datasets.size()==0
  // throws exception or prints warning when adjusting lists.
  void checkAndAdjustInputLists(std::vector<XtcInput::XtcFileName> &filenames, 
                                std::vector<std::string> &datasets) {
    if ((filenames.size()==0) and (datasets.size()==0)) {
      throw XtcInput::NoFilesInDataset(ERR_LOC, "");
    }
    if ((filenames.size()>0) and (datasets.size()>0)) {
      MsgLog(logger, warning, "There are both datasets and filenames " <<
             "specified. Not supported." << std::endl << 
             " Ignoring filenames, first is " << filenames.at(0));
      filenames.resize(0);
    }
    if (datasets.size() > 1) {
      MsgLog(logger, warning, "There is more than one dataset specified. " <<
             " Keeping first, and ignoring others, first other is " << 
             datasets.at(1));
      datasets.resize(1);
    }
  }
  
  void checkFilenames(std::vector<XtcInput::XtcFileName> &filenames) {
    std::set<unsigned> expNums;
    std::set<bool> smallData;
    for (std::vector<XtcInput::XtcFileName>::iterator iter = filenames.begin(); 
         iter != filenames.end(); ++iter) {
      expNums.insert(iter->expNum());
      smallData.insert(iter->small());
    }
    if (expNums.size()>1) {
      MsgLog(logger, warning, "multiple experiment id's detected in xtc filenames.");
    }
    if (smallData.size()>1) {
      throw XtcInput::MixedSmallInDataset(ERR_LOC, "");
    }
  }
  
}

//             ----------------------------------------
//             -- Public Function Member Definitions --
//             ----------------------------------------

namespace XtcInput {

//--------------
// Destructor --
//--------------
DgramReader::~DgramReader ()
{
}

// this is the "run" method used by the Boost.thread
void
DgramReader::operator() ()
try {
  std::vector<XtcFileName> filenames;
  std::vector<std::string> datasets;
  splitIntoXtcFilesAndDatasets(m_files, filenames, datasets);
  checkAndAdjustInputLists(filenames, datasets);

  boost::shared_ptr<RunFileIterI> runFileIter;

  m_liveAvail.reset();
  bool liveMode = false;
  if (datasets.size()==1) {
    IData::Dataset ds(datasets.at(0));
    std::vector<unsigned> runNumbers = ds.runsAsList();
    if (runNumbers.size()==0) {
      throw NoRunsInDataset(ERR_LOC, "");
    }
    if (ds.exists("one-stream")) {
      throw DeprecatedFeature(ERR_LOC, "one-stream has been deprecated. Use stream=k or read small data");
    }
    // use default table name if none was given
    if (m_liveDbConn.empty()) m_liveDbConn = "Server=psdb.slac.stanford.edu;Database=regdb;Uid=regdb_reader";
    if (ds.exists("live")) {
      liveMode = true;
      std::vector<unsigned> streamsFilterAsVector = ds.streamsAsList();
      std::set<unsigned> streamsFilter(streamsFilterAsVector.begin(), 
                                       streamsFilterAsVector.end());
      boost::shared_ptr<RunFileIterLive> 
        runFileIterLive(new RunFileIterLive(runNumbers.begin(), runNumbers.end(),
                                            ds.expID(), streamsFilter,
                                            m_liveTimeout, m_runLiveTimeout, m_liveDbConn, 
                                            m_liveTable, ds.dirName(), ds.exists("smd")));
      runFileIter = runFileIterLive;
    } else {
      // dataset specified, but not live mode. 
      const IData::Dataset::NameList & datasetNameList = ds.files();
      for (IData::Dataset::NameList::const_iterator it = datasetNameList.begin();
           it != datasetNameList.end(); ++it) {
        filenames.push_back(XtcFileName(*it));
      }
    }
  } else {
    // no dataset specified, list of xtc filenames.
  }
  if (not runFileIter) {
    checkFilenames(filenames);
    runFileIter = boost::make_shared<RunFileIterList>(filenames.begin(), filenames.end(), m_mode);
  }


  moveDgramsThroughQueue(runFileIter, liveMode);
  
} catch (const boost::thread_interrupted& ex) {

  // we just stop happily, remove all current datagrams from a queue
  // to make sure there is enough free spave and add end-of-data datagram just in
  // case someone needs it
  m_queue.clear();
  m_queue.push ( Dgram() ) ;
  
} catch ( std::exception& e ) {
  
  // push exception message to a queue which will cause exception in consumer thread
  m_queue.push_exception(e.what());
  
}
  
  void DgramReader::moveDgramsThroughQueue(boost::shared_ptr<RunFileIterI> runFileIter, bool liveMode) {

  if (runFileIter) {

    XtcMergeIterator iter(runFileIter, m_l1OffsetSec, m_firstControlStream, 
                          m_maxStreamClockDiffSec, m_thirdEvent);
    if (liveMode) {
      m_liveAvail = boost::make_shared<XtcInput::LiveAvail>(&iter);
    }
    Dgram dg;
    while ( not boost::this_thread::interruption_requested() ) {
      
      dg = iter.next();
      
      // stop if no datagram
      if (dg.empty()) break;
      
      // move it to the queue
      m_queue.push ( dg ) ;
      
    }
    
  } else {

    MsgLog(logger, warning, "no input data specified");

  }
  if (liveMode) m_liveAvail->mergerAboutToBeDestroyed();
  // tell all we are done
  m_queue.push ( Dgram() ) ;
}
  
} // namespace XtcInput

