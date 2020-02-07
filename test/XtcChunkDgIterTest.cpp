//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class XtcChunkDgIterTest...
//
// Author List:
//      Andy Salnikov
//
//------------------------------------------------------------------------

//-----------------
// C/C++ Headers --
//-----------------
#include <algorithm>
#include <iostream>
#include <stack>
#include <stdio.h>
#include <boost/thread.hpp>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <boost/filesystem.hpp>

//----------------------
// Base Class Headers --
//----------------------
#include "AppUtils/AppBase.h"

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "MsgLogger/MsgLogger.h"
#include "XtcInput/XtcChunkDgIter.h"
#include "XtcInput/Exceptions.h"

//-----------------------------------------------------------------------
// Local Macros, Typedefs, Structures, Unions and Forward Declarations --
//-----------------------------------------------------------------------

//		----------------------------------------
// 		-- Public Function Member Definitions --
//		----------------------------------------

namespace XtcInput {

//
//  Application class
//
class XtcChunkDgIterTest : public AppUtils::AppBase {
public:

  // Constructor
  explicit XtcChunkDgIterTest ( const std::string& appName ) ;

  // destructor
  ~XtcChunkDgIterTest () ;

protected :

  /**
   *  Main method which runs the whole application
   */
  virtual int runApp () ;

  typedef std::pair<int, std::string> FileHandleName;

  // some writers leave open files around as part of test - toClean argument is mechanism
  // to delete after the test is done, otherwise sometimes get nfs errors
  static void writer1(int ndg, std::string fileName, std::stack<FileHandleName> &toClean);
  static void writer2(int ndg, std::string fileName, std::string finalName, int timeout, std::stack<FileHandleName> &toClean);
  static void writer3(int ndg, std::string fileName, std::string finalName, int timeout, std::stack<FileHandleName> &toClean);

  int test1();
  int test2();
  int test3();
  int test4();
  int test5();

  void cleanDir();

private:
  std::stack<FileHandleName> m_toClean;
  std::string m_dirName;
  XtcInput::XtcFileName m_xtcFileNameFinal;
  XtcInput::XtcFileName m_xtcFileNameInProgress;
  static Dgram::ptr makeDgram(size_t payloadSize);
  static int open(std::string fileName);
  static bool checkDg(const boost::shared_ptr<DgHeader>& hptr, bool empty, int payload);
};

//----------------
// Constructors --
//----------------
XtcChunkDgIterTest::XtcChunkDgIterTest ( const std::string& appName )
  : AppUtils::AppBase( appName )
{
  char dirNameBuffer[128];
  strcpy(dirNameBuffer, "unit_test_XtcChunkDgIterTest_XXXXXX");
  if (NULL == mkdtemp(dirNameBuffer)) {
    throw XtcInput::ErrnoException(ERR_LOC, "mkdtemp","null ptr returned");
  }
  m_dirName = std::string(dirNameBuffer);
  struct stat buf;
  if (0 != stat(m_dirName.c_str(), &buf)) {
    throw XtcInput::ErrnoException(ERR_LOC, "stat","trying to stat mkdtemp dir");
  }
  buf.st_mode |= S_IRGRP;
  buf.st_mode |= S_IWGRP;
  if (0 != chmod(m_dirName.c_str(), buf.st_mode)) {
    throw XtcInput::ErrnoException(ERR_LOC, "chmod","trying to change to RW for group and usr");
  }
  std::string expPrefix = "e1";
  unsigned run = 1;
  unsigned stream = 0;
  unsigned chunk = 0;
  bool small = false;
  m_xtcFileNameFinal = XtcInput::XtcFileName(m_dirName, expPrefix, run, stream, chunk, small);
  m_xtcFileNameInProgress = XtcInput::XtcFileName(m_xtcFileNameFinal.path() + ".inprogress");
}

//--------------
// Destructor --
//--------------
XtcChunkDgIterTest::~XtcChunkDgIterTest ()
{
  cleanDir();
  if (m_dirName.size()>3) {
    unsigned tries = 0;
    boost::system::error_code ec;
    do {
      sleep(1);
      tries += 1;
      boost::filesystem::remove_all(m_dirName, ec);
      MsgLog("destructor", info, "trying to remove dir, ec="
             << ec << " tries=" << tries << " dir=" << m_dirName);
    } while ((boost::system::errc::success != ec) and (tries < 10));
    if (boost::system::errc::success != ec) {
      MsgLog("destructor", error, "still unable to remove directory " << m_dirName);
    }
  }
}

void
XtcChunkDgIterTest::cleanDir()
{
  while (m_toClean.size()>0) {
    FileHandleName &fhName = m_toClean.top();
    m_toClean.pop();
    int fd = fhName.first;
    std::string fname = fhName.second;
    if (fd >= 0) {
      if (0 != close(fd)) {
        MsgLog("cleanDir", warning, "close(" << fd << ") for fname=" << fname << " failed");
      }
    }
    if (0 != unlink(fname.c_str())) {
      MsgLog("cleanDir", warning, "unlink(" << fname << ") failed");
    }
  }
}

/**
 *  Main method which runs the whole application
 */
int
XtcChunkDgIterTest::runApp ()
{
  if (0 != test1()) return -1;
  if (0 != test2()) return -1;
  if (0 != test3()) return -1;
  if (0 != test4()) return -1;
  if (0 != test5()) return -1;
  // return 0 on success, other values for error (like main())
  return 0 ;
}

int
XtcChunkDgIterTest::test1()
{
  // Simple test for non-live data reading,
  // write a bunch of datagrams and read them back,
  // check their sizes
  cleanDir();
  MsgLog("test1", info, "running test1");

  std::string fname = m_xtcFileNameFinal.path();
  writer1(5, fname, m_toClean);

  XtcChunkDgIter iter(XtcFileName(fname), 0);
  boost::shared_ptr<DgHeader> hptr;
  hptr = iter.next();
  if (not checkDg(hptr, false, 100)) return -1;
  hptr = iter.next();
  if (not checkDg(hptr, false, 110)) return -1;
  hptr = iter.next();
  if (not checkDg(hptr, false, 120)) return -1;
  hptr = iter.next();
  if (not checkDg(hptr, false, 130)) return -1;
  hptr = iter.next();
  if (not checkDg(hptr, false, 140)) return -1;
  hptr = iter.next();
  if (not checkDg(hptr, true, 0)) return -1;

  return 0;
}

int
XtcChunkDgIterTest::test2()
{
  // Test for hang during live data reading,
  // write ".inprogress" file but do not rename it
  // expect timeout exception from reader

  cleanDir();
  MsgLog("test2", info, "running test2");

  std::string fname = m_xtcFileNameInProgress.path();
  writer1(5, fname, m_toClean);

  unsigned liveTimeout = 3;
  XtcChunkDgIter iter(XtcFileName(fname), liveTimeout);
  boost::shared_ptr<DgHeader> hptr;
  hptr = iter.next();
  if (not checkDg(hptr, false, 100)) return -1;
  hptr = iter.next();
  if (not checkDg(hptr, false, 110)) return -1;
  hptr = iter.next();
  if (not checkDg(hptr, false, 120)) return -1;
  hptr = iter.next();
  if (not checkDg(hptr, false, 130)) return -1;
  hptr = iter.next();
  if (not checkDg(hptr, false, 140)) return -1;
  try {
    hptr = iter.next();
    MsgLog("test2", error, "did not receive expected timeout exception");
    return -1;
  } catch (const XTCLiveTimeout& exc) {
  }

  return 0;
}

int
XtcChunkDgIterTest::test3()
{
  // Test for regular live data reading,
  // write ".inprogress" file in a separate thread
  // rename it to a final ".xtc" name.
  // Writer2 writes complete datagrams.

  cleanDir();
  MsgLog("test3", info, "running test3");

  std::string fname = m_xtcFileNameInProgress.path();
  std::string finalname = m_xtcFileNameFinal.path();

  int ndg = 3;
  int writerTimeout = 3;
  boost::thread thread(writer2, ndg, fname, finalname, writerTimeout, m_toClean);

  sleep(1);

  unsigned iterLiveTimeout = 6;
  XtcChunkDgIter iter(XtcFileName(fname), iterLiveTimeout);
  boost::shared_ptr<DgHeader> hptr;
  hptr = iter.next();
  if (not checkDg(hptr, false, 100)) return -1;
  hptr = iter.next();
  if (not checkDg(hptr, false, 110)) return -1;
  hptr = iter.next();
  if (not checkDg(hptr, false, 120)) return -1;
  hptr = iter.next();
  if (not checkDg(hptr, true, 0)) return -1;

  thread.join();
  return 0;
}

int
XtcChunkDgIterTest::test4()
{
  // Test for regular live data reading,
  // write ".inprogress" file in a separate thread
  // rename it to a final ".xtc" name
  // Writer2 implements "slow" writing so that we can
  // read partial datagrams and wait for complete datagram

  cleanDir();
  MsgLog("test4", info, "running test4");

  std::string fname = m_xtcFileNameInProgress.path();
  std::string finalname = m_xtcFileNameFinal.path();

  boost::thread thread(writer3, 3, fname, finalname, 1, m_toClean);

  sleep(1);

  XtcChunkDgIter iter(XtcFileName(fname), 5);
  boost::shared_ptr<DgHeader> hptr;
  hptr = iter.next();
  if (not checkDg(hptr, false, 100)) return -1;
  hptr = iter.next();
  if (not checkDg(hptr, false, 110)) return -1;
  hptr = iter.next();
  if (not checkDg(hptr, false, 120)) return -1;
  hptr = iter.next();
  if (not checkDg(hptr, true, 0)) return -1;

  thread.join();
  return 0;
}

int
XtcChunkDgIterTest::test5()
{
  // Test for failed transfer,
  // write ".inprogress" file in a separate thread
  // rename it to a final ".inprogress.XXXX"

  cleanDir();
  MsgLog("test5", info, "running test5");

  std::string fname = m_xtcFileNameInProgress.path();
  std::string finalname = fname + ".123";

  boost::thread thread(writer3, 3, fname, finalname, 1, m_toClean);

  sleep(1);

  XtcChunkDgIter iter(XtcFileName(fname), 5);
  boost::shared_ptr<DgHeader> hptr;
  hptr = iter.next();
  if (not checkDg(hptr, false, 100)) return -1;
  hptr = iter.next();
  if (not checkDg(hptr, false, 110)) return -1;
  hptr = iter.next();
  if (not checkDg(hptr, false, 120)) return -1;
  try {
    hptr = iter.next();
    MsgLog("test5", error, "did not receive expected timeout exception");
  } catch (const XTCLiveTimeout& exc) {
  }

  thread.join();
  return 0;
}

bool
XtcChunkDgIterTest::checkDg(const boost::shared_ptr<DgHeader>& hptr, bool empty, int payload)
{
  if (not empty and not hptr) {
    MsgLog("test1", error, "expected non-empty datagram, got empty");
    return false;
  }
  if (empty and hptr) {
    MsgLog("test1", error, "expected empty datagram, got non-empty");
    return false;
  }
  if (not empty) {
    Dgram::ptr dg = hptr->dgram();
    if (dg->xtc.sizeofPayload() != payload) {
      MsgLog("test1", error, "expected payload size " << payload << ", got " << dg->xtc.sizeofPayload());
      return false;
   }
  }
  return true;
}


// function that will write a number of datagrams to output file
void
XtcChunkDgIterTest::writer1(int ndg, std::string fileName, std::stack<FileHandleName> &toClean)
{
  int fd = open(fileName);
  if (fd < 0) return;

  for (int i = 0; i < ndg; ++ i) {

    size_t payloadSize = 10*i + 100;
    Dgram::ptr dg = makeDgram(payloadSize);
    write(fd, (char*)dg.get(), sizeof(Pds::Dgram)+dg->xtc.sizeofPayload());

  }
  close(fd);
  toClean.push(FileHandleName(-1, fileName));
}

// function that will write a number of datagrams to output file then renames it after timeout
void
XtcChunkDgIterTest::writer2(int ndg, std::string fileName, std::string finalName, int timeout, std::stack<FileHandleName> &toClean)
{
  writer1(ndg, fileName, toClean);

  sleep(timeout);
  rename(fileName.c_str(), finalName.c_str());
  toClean.pop();
  toClean.push(FileHandleName(-1, finalName));
}

// function that slowly writes a number of datagrams in "slow" mode
void
XtcChunkDgIterTest::writer3(int ndg, std::string fileName, std::string finalName, int timeout, std::stack<FileHandleName> &toClean)
{
  int fd = open(fileName);
  if (fd < 0) return;

  for (int i = 0; i < ndg; ++ i) {

    size_t payloadSize = 10*i + 100;
    Dgram::ptr dg = makeDgram(payloadSize);

    char* b = (char*)dg.get();
    size_t size = sizeof(Pds::Dgram)+dg->xtc.sizeofPayload();

    size_t off = 0;
    while (off < size) {
      size_t s = size - off;
      if (off == 0) s = 15;
      if (off == 15) s = 40;
      off += write(fd, b + off, s);
      sleep(1);
    }

  }
  close(fd);

  sleep(timeout);
  rename(fileName.c_str(), finalName.c_str());
  toClean.push(FileHandleName(-1, finalName));
}

int
XtcChunkDgIterTest::open(std::string fileName)
{
  int fd = ::open(fileName.c_str(), O_CREAT|O_TRUNC|O_WRONLY|O_SYNC, 0660);
  if (fd < 0) {
    MsgLog("writer", error, "Failed to open output file: " << fileName);
  }
  return fd;
}

Dgram::ptr
XtcChunkDgIterTest::makeDgram(size_t payloadSize)
{
  char* buf = new char[sizeof(Pds::Dgram) + payloadSize];
  Pds::Dgram* dg = (Pds::Dgram*)buf;
  std::fill_n(buf, sizeof(Pds::Dgram) + payloadSize, '\xff');

  dg->seq = Pds::Sequence(Pds::ClockTime(1,1), Pds::TimeStamp());
  dg->env = Pds::Env(1100);
  dg->xtc.damage = Pds::Damage(13);
  dg->xtc.src = Pds::Src();
  dg->xtc.contains = Pds::TypeId(Pds::TypeId::Any, 0);
  dg->xtc.extent = payloadSize+sizeof(Pds::Xtc);

  return Dgram::make_ptr(dg);
}

} // namespace XtcInput


// this defines main()
APPUTILS_MAIN(XtcInput::XtcChunkDgIterTest)
