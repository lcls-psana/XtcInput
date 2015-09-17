//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Test suite case for both L1AcceptsFollowing and CountUpcomingSorted
//
//------------------------------------------------------------------------

//---------------
// C++ Headers --
//---------------
#include "boost/make_shared.hpp"
#include <cassert>

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "XtcInput/L1AcceptsFollowing.h"
#include "FileIO/MockFileIO.h"
#include "XtcInput/Exceptions.h"
#include "XtcInput/CountUpcomingSorted.h"

using namespace XtcInput ;
using namespace FileIO ;


// This test suite uses the MockFileIO by taking the output of 
// xtclinedump for some real datagram headers. First some helper 
// function to deal with taking the parsed output and turning
// into binary buffers to read back

// take a list of 10 int32's and return a 40 long byte vector
std::vector<uint8_t> makeDgBuffer(int32_t *dgData) {
  std::vector<uint8_t> buffer(40);
  int32_t *curInt32 = dgData;
  for (int i = 0; i < 10; ++i) {
    uint8_t *curByte = static_cast<uint8_t*>(static_cast<void *>(curInt32));
    for (int j =0; j < 4; ++j) {
      buffer.at(i*4+j) = *curByte++;
    }
    curInt32++;
  }
  for (int i = 0; i < 10; ++i) {
    int32_t * q = static_cast<int32_t*>(static_cast<void*>(&buffer.at(4*i)));
    assert(dgData[i] == *q);
  }
  return buffer;
}
 
// take a offset, and a  list of 10 inteters, make a 40 long buffer and
// store it in the mapOff2Buffer object, i.e, afterwards 
// mapOff2Buffer[offset] = [a0, a1, ...]
void addOffsetAndDgram(MockFileIO::MapOff2Buffer & mapOff2Buffer, 
                       off_t offset, int a0, int a1, int a2, int a3, 
                       int a4, int a5, int a6, int a7, int a8, int a9) {
  int32_t intBuffer[10];
  intBuffer[0]=a0;
  intBuffer[1]=a1;
  intBuffer[2]=a2;
  intBuffer[3]=a3;
  intBuffer[4]=a4;
  intBuffer[5]=a5;
  intBuffer[6]=a6;
  intBuffer[7]=a7;
  intBuffer[8]=a8;
  intBuffer[9]=a9;
  mapOff2Buffer[offset] = makeDgBuffer(intBuffer);
}

// returns MockFileIO object from first 10 datagrams of the smd
// file for sxrh4315/xtc/smalldata/e642-r0128-s04-c00.smd.xtc.
// at the end of these long lines is the text for some of the
// dgram headers
MockFileIO::MapFname2Off2Buffer makeTestStream(const std::string fname) {
  MockFileIO::MapOff2Buffer mapOff2Buffer;
  addOffsetAndDgram(mapOff2Buffer, 0x00000000,  0x1281AE5F, 0x55ACCCBB, 0x04000000, 0x0001FFFF, 0x00004782, 0x00000000, 0x03000AF2, 0xAC151536, 0x00000001, 0x00014568);    // sv=      Configure ex=0 ev=0 sec=55ACCCBB nano=1281AE5F tcks=0000000 fid=1FFFF 
  addOffsetAndDgram(mapOff2Buffer, 0x0001457C,  0x1B674DBE, 0x55AD0AE9, 0x06000000, 0x0001FFFF, 0x00000080, 0x00000000, 0x03000AF2, 0xAC151536, 0x00000001, 0x000000C8);    // sv=       BeginRun ex=0 ev=0 sec=55AD0AE9 nano=1B674DBE tcks=0000000 fid=1FFFF 
  addOffsetAndDgram(mapOff2Buffer, 0x00014658,  0x27F966E3, 0x55AD0AE9, 0x08000000, 0x0001FFFF, 0x00000000, 0x00000000, 0x03000AF2, 0xAC151536, 0x00000001, 0x0000057C);    // sv=BeginCalibCycle ex=0 ev=0 sec=55AD0AE9 nano=27F966E3 tcks=0000000 fid=1FFFF 
  addOffsetAndDgram(mapOff2Buffer, 0x00014BE8,  0x2A5C42A6, 0x55AD0AE9, 0x0A000000, 0x0001FFFF, 0x80000000, 0x00000000, 0x03000AF2, 0xAC151536, 0x00000001, 0x000000C8);    // sv=         Enable ex=0 ev=0 sec=55AD0AE9 nano=2A5C42A6 tcks=0000000 fid=1FFFF 
  addOffsetAndDgram(mapOff2Buffer, 0x00014CC4,  0x2EB71F0E, 0x55AD0AE9, 0x8C050AFA, 0x9FE5ED4D, 0x00000003, 0x00000000, 0x03000AF2, 0xAC151536, 0x00000001, 0x00003E88);    // sv=       L1Accept ex=1 ev=1 sec=55AD0AE9 nano=2EB71F0E tcks=0050AFA fid=1ED4D 
  addOffsetAndDgram(mapOff2Buffer, 0x00018B60,  0x31B25056, 0x55AD0AE9, 0x8C050CD6, 0x9FF1ED5F, 0x00000003, 0x00000000, 0x03000AF2, 0xAC151536, 0x00000001, 0x00003E88);    // sv=       L1Accept ex=1 ev=1 sec=55AD0AE9 nano=31B25056 tcks=0050CD6 fid=1ED5F 
  addOffsetAndDgram(mapOff2Buffer, 0x0001C9FC,  0x34AD886E, 0x55AD0AE9, 0x8C050E0A, 0x9FFDED71, 0x00000003, 0x00000000, 0x03000AF2, 0xAC151536, 0x00000001, 0x00003E88);    // sv=       L1Accept ex=1 ev=1 sec=55AD0AE9 nano=34AD886E tcks=0050E0A fid=1ED71 
  addOffsetAndDgram(mapOff2Buffer, 0x00020898,  0x37A8A49E, 0x55AD0AE9, 0x8C050B4E, 0xA009ED83, 0x00000003, 0x00000000, 0x03000AF2, 0xAC151536, 0x00000001, 0x00003E88);    // sv=       L1Accept ex=1 ev=1 sec=55AD0AE9 nano=37A8A49E tcks=0050B4E fid=1ED83 
  addOffsetAndDgram(mapOff2Buffer, 0x00024734,  0x3AA3AFB6, 0x55AD0AE9, 0x8C05061C, 0xA015ED95, 0x00000003, 0x00000000, 0x03000AF2, 0xAC151536, 0x00000001, 0x00003E88);    // sv=       L1Accept ex=1 ev=1 sec=55AD0AE9 nano=3AA3AFB6 tcks=005061C fid=1ED95 
  addOffsetAndDgram(mapOff2Buffer, 0x000285D0,  0x020415A5, 0x55AD0AEA, 0x8C050AA6, 0xA021EDA7, 0x00000003, 0x00000000, 0x03000AF2, 0xAC151536, 0x00000001, 0x00003E88);    // sv=       L1Accept ex=1 ev=1 sec=55AD0AEA nano=020415A5 tcks=0050AA6 fid=1EDA7 
  addOffsetAndDgram(mapOff2Buffer, 0x0002C46C,  0x04FF4563, 0x55AD0AEA, 0x8C050D1C, 0xA02DEDB9, 0x00000003, 0x00000000, 0x03000AF2, 0xAC151536, 0x00000001, 0x00003E88);    // sv=       L1Accept ex=1 ev=1 sec=55AD0AEA nano=04FF4563 tcks=0050D1C fid=1EDB9 
  addOffsetAndDgram(mapOff2Buffer, 0x00030308,  0x07FA7A42, 0x55AD0AEA, 0x8C050E0A, 0xA039EDCB, 0x00000003, 0x00000000, 0x03000AF2, 0xAC151536, 0x00000001, 0x00003E88);    // sv=       L1Accept ex=1 ev=1 sec=55AD0AEA nano=07FA7A42 tcks=0050E0A fid=1EDCB 
  addOffsetAndDgram(mapOff2Buffer, 0x000341A4,  0x0AF593A5, 0x55AD0AEA, 0x8C050B08, 0xA045EDDD, 0x00000003, 0x00000000, 0x03000AF2, 0xAC151536, 0x00000001, 0x00003E88);    // sv=       L1Accept ex=1 ev=1 sec=55AD0AEA nano=0AF593A5 tcks=0050B08 fid=1EDDD 
  addOffsetAndDgram(mapOff2Buffer, 0x00038040,  0x0DF09EA7, 0x55AD0AEA, 0x8C0505F2, 0xA051EDEF, 0x00000003, 0x00000000, 0x03000AF2, 0xAC151536, 0x00000001, 0x00003E88);    // sv=       L1Accept ex=1 ev=1 sec=55AD0AEA nano=0DF09EA7 tcks=00505F2 fid=1EDEF 
  addOffsetAndDgram(mapOff2Buffer, 0x0003BEDC,  0x10EBCEB3, 0x55AD0AEA, 0x8C050ADE, 0xA05DEE01, 0x00000003, 0x00000000, 0x03000AF2, 0xAC151536, 0x00000001, 0x00003E88);    // sv=       L1Accept ex=1 ev=1 sec=55AD0AEA nano=10EBCEB3 tcks=0050ADE fid=1EE01 
  addOffsetAndDgram(mapOff2Buffer, 0x0003FD78,  0x13E6FC43, 0x55AD0AEA, 0x8C050D38, 0xA069EE13, 0x00000003, 0x00000000, 0x03000AF2, 0xAC151536, 0x00000001, 0x00003E88);    // sv=       L1Accept ex=1 ev=1 sec=55AD0AEA nano=13E6FC43 tcks=0050D38 fid=1EE13 
  MockFileIO::MapFname2Off2Buffer fnameMap;
  fnameMap[fname] = mapOff2Buffer;
  return fnameMap;
}
  
#define BOOST_TEST_MODULE CountUpcoming
#include <boost/test/included/unit_test.hpp>

// make the MockFileIO object and
// a open file descriptor available to all
// tests in this suite:
struct Fixture {
  Fixture() 
    : fname("/reg/d/psdm/sxr/sxrh4315/xtc/smalldata/e642-r0128-s04-c00.smd.xtc")
  {
    BOOST_TEST_MESSAGE("Setup fixture");
    fnameMap = makeTestStream(fname);
    mapOff2Buffer = &fnameMap[fname];
    fileIO = boost::make_shared<MockFileIO>(fnameMap);
    fd = fileIO->open(fname.c_str(), O_RDONLY);
  }

  ~Fixture() {
    BOOST_TEST_MESSAGE("Teardown fixture");
  }

  std::string fname;
  MockFileIO::MapFname2Off2Buffer fnameMap;
  MockFileIO::MapOff2Buffer *mapOff2Buffer;
  boost::shared_ptr<MockFileIO> fileIO;
  int fd;
};

BOOST_FIXTURE_TEST_SUITE( MockFileSuite, Fixture)

BOOST_AUTO_TEST_CASE( test_L1AcceptOffsetsFollowing )
{
  std::vector<off_t> following = XtcInput::L1AcceptOffsetsFollowing(fd, 0, 100, *fileIO);

  // first four are not L1Accept, and last one is not completely on disk, only its dg header is
  BOOST_CHECK_EQUAL(mapOff2Buffer->size()-4-1, following.size()); 

  following = XtcInput::L1AcceptOffsetsFollowing(fd, 0x00014658, 4, *fileIO);
  BOOST_CHECK_EQUAL(4u, following.size());
}

BOOST_AUTO_TEST_CASE( test_L1AcceptOffsetsFollowingFunctor )
{
  XtcInput::L1AcceptOffsetsFollowingFunctor functor(fd, fileIO);
  std::vector<off_t> following = functor(0, 100);

  // first four are not L1Accept, and last one is not completely on disk, only its dg header is
  BOOST_CHECK_EQUAL(mapOff2Buffer->size()-4-1, following.size()); 

  following = functor(0x00014658, 4);
  BOOST_CHECK_EQUAL(4u, following.size());
}

BOOST_AUTO_TEST_CASE( test_CountUpcoming )
{
  XtcInput::L1AcceptOffsetsFollowingFunctor functor(fd, fileIO);
  XtcInput::CountUpcomingSorted<off_t, XtcInput::L1AcceptOffsetsFollowingFunctor> cnt(functor);
  BOOST_CHECK_EQUAL(3u, cnt.afterUpTo(0x00014BE8, 3)); // start at Enable, should cache 3 L1Accepts
  BOOST_CHECK_EQUAL(3u, cnt.afterUpTo(0x00014BE8, 3)); // will clear cache and reset cache since looks earlier than L1Accepts in cache
  BOOST_CHECK_EQUAL(2u, cnt.afterUpTo(0x00014CC4, 2)); // should just get answer from cache
  BOOST_CHECK_EQUAL(5u, cnt.afterUpTo(0x00014CC4, 5)); // should use cache and read three more with functor
  BOOST_CHECK_EQUAL(5u, cnt.afterUpTo(0x00014CC4, 5)); // test cached value
  BOOST_CHECK_EQUAL(3u, cnt.afterUpTo(0x0001C9FC, 3)); // should clear first two from cache
  BOOST_CHECK_EQUAL(3u, cnt.afterUpTo(0x0, 3));        // clear cache, start at front
  BOOST_CHECK_EQUAL(3u, cnt.afterUpTo(0x0, 3));        // reuse cache
  BOOST_CHECK_EQUAL(2u, cnt.afterUpTo(0x00030308, 2)); // should clear clear cache, start with empty cache, fill with 2
  BOOST_CHECK_EQUAL(2u, cnt.afterUpTo(0x00030308, 2)); 
  BOOST_CHECK_EQUAL(0u, cnt.afterUpTo(0x0003FD78, 2)); // last datagram not present
} 

BOOST_AUTO_TEST_SUITE_END()
