//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class LiveFilesWS...
//
// Author List:
//      Andy Salnikov
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------
#include "XtcInput/LiveFilesWS.h"
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>


//-----------------
// C/C++ Headers --
//-----------------

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "MsgLogger/MsgLogger.h"

#include <curl/curl.h>
#include <curl/easy.h>


//-----------------------------------------------------------------------
// Local Macros, Typedefs, Structures, Unions and Forward Declarations --
//-----------------------------------------------------------------------

namespace fs = boost::filesystem;
namespace ptns = boost::property_tree;

namespace {

  const char* logger = "XtcInput.LiveFilesWS";

}

//		----------------------------------------
// 		-- Public Function Member Definitions --
//		----------------------------------------

namespace XtcInput {

//----------------
// Constructors --
//----------------
LiveFilesWS::LiveFilesWS (const std::string& wsURL, const std::string& dir, bool small)
  : m_wsURL(wsURL)
  , m_dir(dir)
  , m_small(small)
{
  if (m_small) {
    fs::path dirpath(m_dir);
    dirpath /= "smalldata";
    m_dir = dirpath.string();
  }
}

//--------------
// Destructor --
//--------------
LiveFilesWS::~LiveFilesWS ()
{
}


size_t __LiveFilesWS_callback__(char* buf, size_t size, size_t nmemb, void* pout) {
  size_t nbytes = size*nmemb;
  ((std::string*)pout)->append((char*)buf, nbytes);
  return nbytes;
}

/**
 *  @brief Returns the list of files for given run
 *
 *  @param[in] expId    Experiment id
 *  @param[in] run      Run number
 */
std::vector<XtcFileName>
LiveFilesWS::files(const std::string& expName, unsigned run)
{
  MsgLog(logger, debug, "LiveFilesWS::files - querying the web service " << m_wsURL << " for file for exp " << expName << " run=" << run << " rooted at folder " << m_dir << " with small data " << m_small);
  std::vector<XtcFileName> result;
  try {
      std::ostringstream stringStream;
      stringStream << m_wsURL << "/lgbk/" << expName << "/ws/" << run << "/files_for_live_mode";
      std::string acturl = stringStream.str();
      CURL *curl = curl_easy_init();
      if(curl) {
        std::string sresp;
        char errbuf[CURL_ERROR_SIZE];
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &__LiveFilesWS_callback__);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &sresp);
        curl_easy_setopt(curl, CURLOPT_URL, acturl.c_str());
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        //curl_easy_setopt(curl, CURLOPT_TRANSFERTEXT, 1L);

        sresp.clear();
        CURLcode res = curl_easy_perform(curl);
        MsgLog(logger, debug, "In request sresp:" << sresp.substr(0,200) << "...\n");

        if(CURLE_OK == res) {
          long response_code;
          curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
          if(response_code == 200) {
              MsgLog(logger, debug, "Received a 200 response from the server sresp.size=" << sresp.size());
              std::stringstream ss;
              ss << sresp;
              ptns::ptree rj;
              ptns::read_json(ss, rj);
              MsgLog(logger, debug, "After parsing the JSON from the server");
              BOOST_FOREACH(const ptns::ptree::value_type &v, rj.get_child("value.")) {
                  fs::path dbPath(v.second.get<std::string>(""));
                  MsgLog(logger, debug, "LiveFilesWS::files - Got file " << dbPath << " for exp " << expName << " run=" << run);
                  XtcFileName xtcBaseName(dbPath.filename().string());
                  if (m_small) {
                      xtcBaseName = XtcFileName(xtcBaseName.smallBasename());
                  }
                  fs::path path(m_dir);
                  path /= fs::path(xtcBaseName.path());
                  XtcFileName fname(path.string());
                  MsgLog(logger, debug, "LiveFilesDB::files - computed path " << fname.path());
                  result.push_back(fname);
              }
          }
          else {
              MsgLog(logger, error, "Got an invalid response code from the server " << response_code << " for exp " << expName << " run=" << run << " when using url " << acturl);
              return result;
          }
      } else {
          MsgLog(logger, error, "Got an error response from the server - error " << errbuf << " for exp " << expName << " run=" << run);
          return result;
      }
        curl_easy_cleanup(curl); // always cleanup
    } else {
        MsgLog(logger, error, "Cannot initialize curl when getting live mode files for exp " << expName << " run=" << run);
        return result;
    }
  } catch(std::string ex){
      MsgLog(logger, error, "LiveFilesWS::files - Exception getting files from server for exp " << expName << " run=" << run);
      return result;
  }
  return result;
}

} // namespace XtcInput
