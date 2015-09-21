//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Test suite for StreamAvail class.
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
#include "FileIO/MockFileIO.h"
#include "XtcInput/Exceptions.h"
#include "XtcInput/StreamAvail.h"

using namespace XtcInput ;
using namespace FileIO ;

struct OffsetDgHeader {
  off64_t offset;
  uint32_t dgHeader[10];
};

// makes 40 long vector of bytes from 10 int32's
std::vector<uint8_t> fromDgHeader(uint32_t *dgHeader) {
  std::vector<uint8_t> buffer(40);
  for (int i = 0; i < 10; ++i) {
    uint8_t *curByte = static_cast<uint8_t*>(static_cast<void *>(&dgHeader[i]));
    for (int j =0; j < 4; ++j) {
      buffer.at(i*4+j) = *curByte++;
    }
  }
  for (int i = 0; i < 10; ++i) {
    assert(dgHeader[i] == *(static_cast<uint32_t*>(static_cast<void*>(&buffer.at(4*i)))));
  }
  return buffer;
}

// /reg/d/psdm/sxr/sxrh4315/xtc/smalldata/e642-r0128-s04-c00.smd.xtc
OffsetDgHeader chunk0MockData[] = {
  {0x00000000,{ 0x1281AE5F, 0x55ACCCBB, 0x04000000, 0x0001FFFF, 0x00004782, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00014568}}, //dg=    1 offset=0x00000000 tp=Event sv=      Configure ex=0 ev=0 sec=55ACCCBB nano=1281AE5F tcks=0000000 fid=1FFFF ctrl=04 vec=0000 env=00004782  dgHeaderHex={ 0x1281AE5F, 0x55ACCCBB, 0x04000000, 0x0001FFFF, 0x00004782, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00014568}
  {0x0001457C,{ 0x1B674DBE, 0x55AD0AE9, 0x06000000, 0x0001FFFF, 0x00000080, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x000000C8}}, //dg=    2 offset=0x0001457C tp=Event sv=       BeginRun ex=0 ev=0 sec=55AD0AE9 nano=1B674DBE tcks=0000000 fid=1FFFF ctrl=06 vec=0000 env=00000080  dgHeaderHex={ 0x1B674DBE, 0x55AD0AE9, 0x06000000, 0x0001FFFF, 0x00000080, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x000000C8}
  {0x00014658,{ 0x27F966E3, 0x55AD0AE9, 0x08000000, 0x0001FFFF, 0x00000000, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x0000057C}}, //dg=    3 offset=0x00014658 tp=Event sv=BeginCalibCycle ex=0 ev=0 sec=55AD0AE9 nano=27F966E3 tcks=0000000 fid=1FFFF ctrl=08 vec=0000 env=00000000  dgHeaderHex={ 0x27F966E3, 0x55AD0AE9, 0x08000000, 0x0001FFFF, 0x00000000, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x0000057C}
  {0x00014BE8,{ 0x2A5C42A6, 0x55AD0AE9, 0x0A000000, 0x0001FFFF, 0x80000000, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x000000C8}}, //dg=    4 offset=0x00014BE8 tp=Event sv=         Enable ex=0 ev=0 sec=55AD0AE9 nano=2A5C42A6 tcks=0000000 fid=1FFFF ctrl=0A vec=0000 env=80000000  dgHeaderHex={ 0x2A5C42A6, 0x55AD0AE9, 0x0A000000, 0x0001FFFF, 0x80000000, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x000000C8}
  {0x00014CC4,{ 0x30B40715, 0x55AD0AE9, 0x8C050E18, 0x9FEDED59, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E88}}, //dg=    5 offset=0x00014CC4 tp=Event sv=       L1Accept ex=1 ev=1 sec=55AD0AE9 nano=30B40715 tcks=0050E18 fid=1ED59 ctrl=8C vec=4FF6 env=00000003  dgHeaderHex={ 0x30B40715, 0x55AD0AE9, 0x8C050E18, 0x9FEDED59, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E88}
  {0x00018B60,{ 0x33AF0BAC, 0x55AD0AE9, 0x8C050892, 0x9FF9ED6B, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E88}}, //dg=    6 offset=0x00018B60 tp=Event sv=       L1Accept ex=1 ev=1 sec=55AD0AE9 nano=33AF0BAC tcks=0050892 fid=1ED6B ctrl=8C vec=4FFC env=00000003  dgHeaderHex={ 0x33AF0BAC, 0x55AD0AE9, 0x8C050892, 0x9FF9ED6B, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E88}
  {0x0001C9FC,{ 0x36AA2AD5, 0x55AD0AE9, 0x8C050670, 0xA005ED7D, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E88}}, //dg=    7 offset=0x0001C9FC tp=Event sv=       L1Accept ex=1 ev=1 sec=55AD0AE9 nano=36AA2AD5 tcks=0050670 fid=1ED7D ctrl=8C vec=5002 env=00000003  dgHeaderHex={ 0x36AA2AD5, 0x55AD0AE9, 0x8C050670, 0xA005ED7D, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E88}
  {0x00020898,{ 0x39A55A48, 0x55AD0AE9, 0x8C050BA2, 0xA011ED8F, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E88}}, //dg=    8 offset=0x00020898 tp=Event sv=       L1Accept ex=1 ev=1 sec=55AD0AE9 nano=39A55A48 tcks=0050BA2 fid=1ED8F ctrl=8C vec=5008 env=00000003  dgHeaderHex={ 0x39A55A48, 0x55AD0AE9, 0x8C050BA2, 0xA011ED8F, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E88}
  {0x00024734,{ 0x0105C303, 0x55AD0AEA, 0x8C050D70, 0xA01DEDA1, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E88}}, //dg=    9 offset=0x00024734 tp=Event sv=       L1Accept ex=1 ev=1 sec=55AD0AEA nano=0105C303 tcks=0050D70 fid=1EDA1 ctrl=8C vec=500E env=00000003  dgHeaderHex={ 0x0105C303, 0x55AD0AEA, 0x8C050D70, 0xA01DEDA1, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E88}
  {0x000285D0,{ 0x0400F8CE, 0x55AD0AEA, 0x8C050E26, 0xA029EDB3, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E88}}, //dg=   10 offset=0x000285D0 tp=Event sv=       L1Accept ex=1 ev=1 sec=55AD0AEA nano=0400F8CE tcks=0050E26 fid=1EDB3 ctrl=8C vec=5014 env=00000003  dgHeaderHex={ 0x0400F8CE, 0x55AD0AEA, 0x8C050E26, 0xA029EDB3, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E88}
  {0x0002C46C,{ 0x06FBFDB8, 0x55AD0AEA, 0x8C0508AE, 0xA035EDC5, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E88}}, //dg=   11 offset=0x0002C46C tp=Event sv=       L1Accept ex=1 ev=1 sec=55AD0AEA nano=06FBFDB8 tcks=00508AE fid=1EDC5 ctrl=8C vec=501A env=00000003  dgHeaderHex={ 0x06FBFDB8, 0x55AD0AEA, 0x8C0508AE, 0xA035EDC5, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E88}
  {0x00030308,{ 0x09F71CDD, 0x55AD0AEA, 0x8C0506D2, 0xA041EDD7, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E88}}, //dg=   12 offset=0x00030308 tp=Event sv=       L1Accept ex=1 ev=1 sec=55AD0AEA nano=09F71CDD tcks=00506D2 fid=1EDD7 ctrl=8C vec=5020 env=00000003  dgHeaderHex={ 0x09F71CDD, 0x55AD0AEA, 0x8C0506D2, 0xA041EDD7, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E88}
  {0x000341A4,{ 0x0CF24971, 0x55AD0AEA, 0x8C050BDA, 0xA04DEDE9, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E88}}, //dg=   13 offset=0x000341A4 tp=Event sv=       L1Accept ex=1 ev=1 sec=55AD0AEA nano=0CF24971 tcks=0050BDA fid=1EDE9 ctrl=8C vec=5026 env=00000003  dgHeaderHex={ 0x0CF24971, 0x55AD0AEA, 0x8C050BDA, 0xA04DEDE9, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E88}
  {0x00038040,{ 0x0FED7EED, 0x55AD0AEA, 0x8C050D70, 0xA059EDFB, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E88}}, //dg=   14 offset=0x00038040 tp=Event sv=       L1Accept ex=1 ev=1 sec=55AD0AEA nano=0FED7EED tcks=0050D70 fid=1EDFB ctrl=8C vec=502C env=00000003  dgHeaderHex={ 0x0FED7EED, 0x55AD0AEA, 0x8C050D70, 0xA059EDFB, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E88}
  {0x0003BEDC,{ 0x12E8B1DD, 0x55AD0AEA, 0x8C050E5E, 0xA065EE0D, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E88}}, //dg=   15 offset=0x0003BEDC tp=Event sv=       L1Accept ex=1 ev=1 sec=55AD0AEA nano=12E8B1DD tcks=0050E5E fid=1EE0D ctrl=8C vec=5032 env=00000003  dgHeaderHex={ 0x12E8B1DD, 0x55AD0AEA, 0x8C050E5E, 0xA065EE0D, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E88}
  // next 0-up index is 15
  {-1,{0,0,0,0,0,0,0,0,0,0}},
  // next 0-up index is 16
  {0x2482D574,{ 0x0A566B0E, 0x55AD12CF, 0x0C050AEC, 0xA02908B7, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E88}}, //dg=38237 offset=0x2482D574 tp=Event sv=       L1Accept ex=0 ev=1 sec=55AD12CF nano=0A566B0E tcks=0050AEC fid=108B7 ctrl=0C vec=5014 env=00000003  dgHeaderHex={ 0x0A566B0E, 0x55AD12CF, 0x0C050AEC, 0xA02908B7, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E88}
  {0x24831410,{ 0x0D51DC06, 0x55AD12CF, 0x8C050830, 0xA03508C9, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E88}}, //dg=38238 offset=0x24831410 tp=Event sv=       L1Accept ex=1 ev=1 sec=55AD12CF nano=0D51DC06 tcks=0050830 fid=108C9 ctrl=8C vec=501A env=00000003  dgHeaderHex={ 0x0D51DC06, 0x55AD12CF, 0x8C050830, 0xA03508C9, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E88}
  {0x248352AC,{ 0x104D6996, 0x55AD12CF, 0x0C050E6C, 0xA04108DB, 0x00000003, 0x00004000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E88}}, //dg=38239 offset=0x248352AC tp=Event sv=       L1Accept ex=0 ev=1 sec=55AD12CF nano=104D6996 tcks=0050E6C fid=108DB ctrl=0C vec=5020 env=00000003  dgHeaderHex={ 0x104D6996, 0x55AD12CF, 0x0C050E6C, 0xA04108DB, 0x00000003, 0x00004000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E88}
  {0x24839148,{ 0x1348BF75, 0x55AD12CF, 0x8C050D46, 0xA04D08ED, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E88}}, //dg=38240 offset=0x24839148 tp=Event sv=       L1Accept ex=1 ev=1 sec=55AD12CF nano=1348BF75 tcks=0050D46 fid=108ED ctrl=8C vec=5026 env=00000003  dgHeaderHex={ 0x1348BF75, 0x55AD12CF, 0x8C050D46, 0xA04D08ED, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E88}
  {0x2483CFE4,{ 0x16441985, 0x55AD12CF, 0x0C050A28, 0xA05908FF, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E88}}, //dg=38241 offset=0x2483CFE4 tp=Event sv=       L1Accept ex=0 ev=1 sec=55AD12CF nano=16441985 tcks=0050A28 fid=108FF ctrl=0C vec=502C env=00000003  dgHeaderHex={ 0x16441985, 0x55AD12CF, 0x0C050A28, 0xA05908FF, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E88}
  {0x24840E80,{ 0x2E849EC3, 0x55AD12CF, 0x0B000000, 0x0001FFFF, 0x00000000, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x000000C8}}, //dg=38242 offset=0x24840E80 tp=Event sv=        Disable ex=0 ev=0 sec=55AD12CF nano=2E849EC3 tcks=0000000 fid=1FFFF ctrl=0B vec=0000 env=00000000  dgHeaderHex={ 0x2E849EC3, 0x55AD12CF, 0x0B000000, 0x0001FFFF, 0x00000000, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x000000C8}
  // next 0-up index is 22
  {612634460,{0,0,0,0,0,0,0,0,0,0}}, // filelength
};

size_t chunk0MockDataLen = sizeof(chunk0MockData)/sizeof(OffsetDgHeader);

// /reg/d/psdm/sxr/sxrh4315/xtc/smalldata/e642-r0128-s04-c01.smd.xtc
OffsetDgHeader chunk1MockData[] = {
  {0x00000000, { 0x317F9237, 0x55AD12CF, 0x0A000000, 0x0001FFFF, 0x80000000, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x000000C8}}, //dg=    1 offset=0x00000000 tp=Event sv=         Enable ex=0 ev=0 sec=55AD12CF nano=317F9237 tcks=0000000 fid=1FFFF ctrl=0A vec=0000 env=80000000  dgHeaderHex={ 0x317F9237, 0x55AD12CF, 0x0A000000, 0x0001FFFF, 0x80000000, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x000000C8}
  {0x000000DC, { 0x3594027E, 0x55AD12CF, 0x8C050726, 0xA06509BC, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E7C}}, //dg=    2 offset=0x000000DC tp=Event sv=       L1Accept ex=1 ev=1 sec=55AD12CF nano=3594027E tcks=0050726 fid=109BC ctrl=8C vec=5032 env=00000003  dgHeaderHex={ 0x3594027E, 0x55AD12CF, 0x8C050726, 0xA06509BC, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E7C}
  {0x00003F6C, { 0x388F93DE, 0x55AD12CF, 0x8C050E96, 0xA07109CE, 0x00000003, 0x00004000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E7C}}, //dg=    3 offset=0x00003F6C tp=Event sv=       L1Accept ex=1 ev=1 sec=55AD12CF nano=388F93DE tcks=0050E96 fid=109CE ctrl=8C vec=5038 env=00000003  dgHeaderHex={ 0x388F93DE, 0x55AD12CF, 0x8C050E96, 0xA07109CE, 0x00000003, 0x00004000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E7C}
  {0x00007DFC, { 0x3B8AEB03, 0x55AD12CF, 0x8C050D9A, 0xA07D09E0, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E7C}}, //dg=    4 offset=0x00007DFC tp=Event sv=       L1Accept ex=1 ev=1 sec=55AD12CF nano=3B8AEB03 tcks=0050D9A fid=109E0 ctrl=8C vec=503E env=00000003  dgHeaderHex={ 0x3B8AEB03, 0x55AD12CF, 0x8C050D9A, 0xA07D09E0, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E7C}
  {0x0000BC8C, { 0x02EB7821, 0x55AD12D0, 0x8C050B16, 0xA08909F2, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E7C}}, //dg=    5 offset=0x0000BC8C tp=Event sv=       L1Accept ex=1 ev=1 sec=55AD12D0 nano=02EB7821 tcks=0050B16 fid=109F2 ctrl=8C vec=5044 env=00000003  dgHeaderHex={ 0x02EB7821, 0x55AD12D0, 0x8C050B16, 0xA08909F2, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E7C}
  {0x0000FB1C, { 0x05E6E2CD, 0x55AD12D0, 0x8C050750, 0xA0950A04, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E7C}}, //dg=    6 offset=0x0000FB1C tp=Event sv=       L1Accept ex=1 ev=1 sec=55AD12D0 nano=05E6E2CD tcks=0050750 fid=10A04 ctrl=8C vec=504A env=00000003  dgHeaderHex={ 0x05E6E2CD, 0x55AD12D0, 0x8C050750, 0xA0950A04, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E7C}
  {0x000139AC, { 0x08E274D9, 0x55AD12D0, 0x8C050E5E, 0xA0A10A16, 0x00000003, 0x00004000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E7C}}, //dg=    7 offset=0x000139AC tp=Event sv=       L1Accept ex=1 ev=1 sec=55AD12D0 nano=08E274D9 tcks=0050E5E fid=10A16 ctrl=8C vec=5050 env=00000003  dgHeaderHex={ 0x08E274D9, 0x55AD12D0, 0x8C050E5E, 0xA0A10A16, 0x00000003, 0x00004000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E7C}
  {0x0001783C, { 0x0BDDC97D, 0x55AD12D0, 0x8C050DA8, 0xA0AD0A28, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E7C}}, //dg=    8 offset=0x0001783C tp=Event sv=       L1Accept ex=1 ev=1 sec=55AD12D0 nano=0BDDC97D tcks=0050DA8 fid=10A28 ctrl=8C vec=5056 env=00000003  dgHeaderHex={ 0x0BDDC97D, 0x55AD12D0, 0x8C050DA8, 0xA0AD0A28, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E7C}
  {0x0001B6CC, { 0x0ED9231C, 0x55AD12D0, 0x8C050B4E, 0xA0B90A3A, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E7C}}, //dg=    9 offset=0x0001B6CC tp=Event sv=       L1Accept ex=1 ev=1 sec=55AD12D0 nano=0ED9231C tcks=0050B4E fid=10A3A ctrl=8C vec=505C env=00000003  dgHeaderHex={ 0x0ED9231C, 0x55AD12D0, 0x8C050B4E, 0xA0B90A3A, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E7C}
  {0x0001F55C, { 0x11D490A3, 0x55AD12D0, 0x8C050788, 0xA0C50A4C, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E7C}}, //dg=   10 offset=0x0001F55C tp=Event sv=       L1Accept ex=1 ev=1 sec=55AD12D0 nano=11D490A3 tcks=0050788 fid=10A4C ctrl=8C vec=5062 env=00000003  dgHeaderHex={ 0x11D490A3, 0x55AD12D0, 0x8C050788, 0xA0C50A4C, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E7C}
  {0x000233EC, { 0x14D02035, 0x55AD12D0, 0x8C050EA4, 0xA0D10A5E, 0x00000003, 0x00004000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E7C}}, //dg=   11 offset=0x000233EC tp=Event sv=       L1Accept ex=1 ev=1 sec=55AD12D0 nano=14D02035 tcks=0050EA4 fid=10A5E ctrl=8C vec=5068 env=00000003  dgHeaderHex={ 0x14D02035, 0x55AD12D0, 0x8C050EA4, 0xA0D10A5E, 0x00000003, 0x00004000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E7C}
  {0x0002727C, { 0x17CB7434, 0x55AD12D0, 0x0C050DC4, 0xA0DD0A70, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E7C}}, //dg=   12 offset=0x0002727C tp=Event sv=       L1Accept ex=0 ev=1 sec=55AD12D0 nano=17CB7434 tcks=0050DC4 fid=10A70 ctrl=0C vec=506E env=00000003  dgHeaderHex={ 0x17CB7434, 0x55AD12D0, 0x0C050DC4, 0xA0DD0A70, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E7C}
  {0x0002B10C, { 0x1AC6CF59, 0x55AD12D0, 0x8C050B24, 0xA0E90A82, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E7C}}, //dg=   13 offset=0x0002B10C tp=Event sv=       L1Accept ex=1 ev=1 sec=55AD12D0 nano=1AC6CF59 tcks=0050B24 fid=10A82 ctrl=8C vec=5074 env=00000003  dgHeaderHex={ 0x1AC6CF59, 0x55AD12D0, 0x8C050B24, 0xA0E90A82, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E7C}
  {0x0002EF9C, { 0x1DC23E84, 0x55AD12D0, 0x0C0507DC, 0xA0F50A94, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E7C}}, //dg=   14 offset=0x0002EF9C tp=Event sv=       L1Accept ex=0 ev=1 sec=55AD12D0 nano=1DC23E84 tcks=00507DC fid=10A94 ctrl=0C vec=507A env=00000003  dgHeaderHex={ 0x1DC23E84, 0x55AD12D0, 0x0C0507DC, 0xA0F50A94, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E7C}
  {0x00032E2C, { 0x20BDCB64, 0x55AD12D0, 0x8C050E96, 0xA1010AA6, 0x00000003, 0x00004000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E7C}}, //dg=   15 offset=0x00032E2C tp=Event sv=       L1Accept ex=1 ev=1 sec=55AD12D0 nano=20BDCB64 tcks=0050E96 fid=10AA6 ctrl=8C vec=5080 env=00000003  dgHeaderHex={ 0x20BDCB64, 0x55AD12D0, 0x8C050E96, 0xA1010AA6, 0x00000003, 0x00004000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E7C}
  // next 0-up index is 15
  {-1,{0,0,0,0,0,0,0,0,0,0}},
  // next 0-up index is 16
  {0x0491B75C, { 0x16C716F0, 0x55AD13BF, 0x8C050E5E, 0x80CC5A78, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E7C}}, //dg= 4789 offset=0x0491B75C tp=Event sv=       L1Accept ex=1 ev=1 sec=55AD13BF nano=16C716F0 tcks=0050E5E fid=05A78 ctrl=8C vec=4066 env=00000003  dgHeaderHex={ 0x16C716F0, 0x55AD13BF, 0x8C050E5E, 0x80CC5A78, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E7C}
  {0x0491F5EC, { 0x19C219BA, 0x55AD13BF, 0x0C050884, 0x80D85A8A, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E7C}}, //dg= 4790 offset=0x0491F5EC tp=Event sv=       L1Accept ex=0 ev=1 sec=55AD13BF nano=19C219BA tcks=0050884 fid=05A8A ctrl=0C vec=406C env=00000003  dgHeaderHex={ 0x19C219BA, 0x55AD13BF, 0x0C050884, 0x80D85A8A, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E7C}
  {0x0492347C, { 0x1CBD3E4B, 0x55AD13BF, 0x8C0505C8, 0x80E45A9C, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E7C}}, //dg= 4791 offset=0x0492347C tp=Event sv=       L1Accept ex=1 ev=1 sec=55AD13BF nano=1CBD3E4B tcks=00505C8 fid=05A9C ctrl=8C vec=4072 env=00000003  dgHeaderHex={ 0x1CBD3E4B, 0x55AD13BF, 0x8C0505C8, 0x80E45A9C, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E7C}
  {0x0492730C, { 0x1FB86816, 0x55AD13BF, 0x8C050B24, 0x80F05AAE, 0x00000003, 0x00004000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E7C}}, //dg= 4792 offset=0x0492730C tp=Event sv=       L1Accept ex=1 ev=1 sec=55AD13BF nano=1FB86816 tcks=0050B24 fid=05AAE ctrl=8C vec=4078 env=00000003  dgHeaderHex={ 0x1FB86816, 0x55AD13BF, 0x8C050B24, 0x80F05AAE, 0x00000003, 0x00004000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E7C}
  {0x0492B19C, { 0x22B3A152, 0x55AD13BF, 0x0C050D38, 0x80FC5AC0, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E7C}}, //dg= 4793 offset=0x0492B19C tp=Event sv=       L1Accept ex=0 ev=1 sec=55AD13BF nano=22B3A152 tcks=0050D38 fid=05AC0 ctrl=0C vec=407E env=00000003  dgHeaderHex={ 0x22B3A152, 0x55AD13BF, 0x0C050D38, 0x80FC5AC0, 0x00000003, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x00003E7C}
  {0x0492F02C, { 0x005081D7, 0x55AD13C0, 0x0B000000, 0x0001FFFF, 0x00000000, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x000000C8}}, //dg= 4794 offset=0x0492F02C tp=Event sv=        Disable ex=0 ev=0 sec=55AD13C0 nano=005081D7 tcks=0000000 fid=1FFFF ctrl=0B vec=0000 env=00000000  dgHeaderHex={ 0x005081D7, 0x55AD13C0, 0x0B000000, 0x0001FFFF, 0x00000000, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x000000C8}
  {0x0492F108, { 0x034B8130, 0x55AD13C0, 0x09000000, 0x0001FFFF, 0x00000000, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x000000C8}}, //dg= 4795 offset=0x0492F108 tp=Event sv=  EndCalibCycle ex=0 ev=0 sec=55AD13C0 nano=034B8130 tcks=0000000 fid=1FFFF ctrl=09 vec=0000 env=00000000  dgHeaderHex={ 0x034B8130, 0x55AD13C0, 0x09000000, 0x0001FFFF, 0x00000000, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x000000C8}
  {0x0492F1E4, { 0x064661C5, 0x55AD13C0, 0x07000000, 0x0001FFFF, 0x00000000, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x000000C8}}, //dg= 4796 offset=0x0492F1E4 tp=Event sv=         EndRun ex=0 ev=0 sec=55AD13C0 nano=064661C5 tcks=0000000 fid=1FFFF ctrl=07 vec=0000 env=00000000  dgHeaderHex={ 0x064661C5, 0x55AD13C0, 0x07000000, 0x0001FFFF, 0x00000000, 0x00000000, 0x03000858, 0xAC151583, 0x00000001, 0x000000C8}
  // next 0-up index is 24
  {76739264, {0,0,0,0,0,0,0,0,0,0}}
};

size_t chunk1MockDataLen = sizeof(chunk1MockData)/sizeof(OffsetDgHeader);

// indicies into mockdata
int NumFirstPart[2] = {13, 13};
int StartLastPart[2] = {16, 16};

// test class for live testing
class LiveUpdate {
public:
  LiveUpdate(const std::string &chunk0,
             const std::string &chunk1,
             OffsetDgHeader *chunk0MockData,
             OffsetDgHeader *chunk1MockData,
             size_t chunk0MockDataLen,
             size_t chunk1MockDataLen) 
  {
    m_chunkNames.push_back(chunk0);
    m_chunkNames.push_back(chunk1);
    std::vector<OffsetDgHeader> chunk0mock, chunk1mock;
    for (int idx=0; idx < int(chunk0MockDataLen); ++idx) chunk0mock.push_back(chunk0MockData[idx]);
    for (int idx=0; idx < int(chunk1MockDataLen); ++idx) chunk1mock.push_back(chunk1MockData[idx]);
    m_mockData.push_back(chunk0mock);
    m_mockData.push_back(chunk1mock);
    m_nextOffset.push_back(-1);
    m_nextOffset.push_back(-1);    
    m_fileIO = boost::make_shared<MockFileIO>(m_fnameMap);
  };

  boost::shared_ptr<MockFileIO> getFileIO() { return m_fileIO; }

  void writeFirstPart(int chunk) {
    MockFileIO::MapOff2Buffer mapOff;
    for (int idx = 0; idx < NumFirstPart[chunk]; ++idx) {
      off_t offset = m_mockData.at(chunk).at(idx).offset;
      m_nextOffset.at(chunk) = m_mockData.at(chunk).at(idx+1).offset;
      mapOff[offset] = fromDgHeader(m_mockData.at(chunk).at(idx).dgHeader);
    }
    m_fnameMap[m_chunkNames.at(chunk) + ".inprogress"] = mapOff;
  }
    
  void writeSecondPart(int chunk) {
    MockFileIO::MapOff2Buffer &mapOff = m_fnameMap[m_chunkNames.at(chunk) + ".inprogress"];
    off_t offsetForPayload = -1;
    for (unsigned idx = StartLastPart[chunk]; idx < m_mockData.at(chunk).size()-1; ++idx) {
      off_t dgSize = m_mockData.at(chunk).at(idx+1).offset - m_mockData.at(chunk).at(idx).offset;
      mapOff[m_nextOffset.at(chunk)] = fromDgHeader(m_mockData.at(chunk).at(idx).dgHeader);
      offsetForPayload = m_nextOffset.at(chunk) + 40;
      m_nextOffset.at(chunk) += dgSize;
    }
    // want to add a buffer for the payload of the last entry
    std::vector<uint8_t> buffer(m_nextOffset.at(chunk)-offsetForPayload);
    mapOff[offsetForPayload] = buffer;
    m_fnameMap[m_chunkNames.at(chunk)] = mapOff;
  }

  off_t sizeAfterSecondWrite(int chunk) {
    return m_nextOffset.at(chunk);
  }

private:
  std::vector<std::string> m_chunkNames;
  std::vector< std::vector<OffsetDgHeader> > m_mockData;
  std::vector<off_t> m_nextOffset;
  MockFileIO::MapFname2Off2Buffer m_fnameMap;
  boost::shared_ptr<MockFileIO> m_fileIO;
};

#define BOOST_TEST_MODULE CountStreamAvail
#include <boost/test/included/unit_test.hpp>

struct Fixture {
  Fixture() 
    : chunk0("/reg/d/psdm/sxr/sxrh4315/xtc/smalldata/e642-r0128-s04-c00.smd.xtc")
    , chunk1("/reg/d/psdm/sxr/sxrh4315/xtc/smalldata/e642-r0128-s04-c01.smd.xtc")
    , lu(chunk0, chunk1,
         chunk0MockData, chunk1MockData, 
         chunk0MockDataLen, chunk1MockDataLen)
  {
    chunk0inprogress = chunk0 + ".inprogress";
    chunk1inprogress = chunk1 + ".inprogress";
  }
  const std::string chunk0;
  const std::string chunk1;
  std::string chunk0inprogress;
  std::string chunk1inprogress;

  LiveUpdate lu;
};

BOOST_FIXTURE_TEST_SUITE( CountStreamAvailSuite, Fixture)

BOOST_AUTO_TEST_CASE( test_HelperClass )
{
  boost::shared_ptr<MockFileIO> fileIO = lu.getFileIO();
  BOOST_CHECK_EQUAL(-1, fileIO->open(chunk0.c_str(),O_RDONLY));
  BOOST_CHECK_EQUAL(-1, fileIO->open(chunk0inprogress.c_str(),O_RDONLY));
  lu.writeFirstPart(0);

  BOOST_CHECK_EQUAL(-1, fileIO->open(chunk0.c_str(),O_RDONLY));
  BOOST_CHECK_EQUAL(true, -1 != fileIO->open(chunk0inprogress.c_str(),O_RDONLY));
  BOOST_CHECK_EQUAL(-1, fileIO->open(chunk1.c_str(),O_RDONLY));

  lu.writeFirstPart(1);
  BOOST_CHECK_EQUAL(-1, fileIO->open(chunk1.c_str(),O_RDONLY));
  BOOST_CHECK_EQUAL(true, -1 != fileIO->open(chunk1inprogress.c_str(),O_RDONLY));
}

BOOST_AUTO_TEST_CASE( test_HelperClass2 )
{
  boost::shared_ptr<MockFileIO> fileIO = lu.getFileIO();
  lu.writeFirstPart(0);
  lu.writeFirstPart(1);
  lu.writeSecondPart(0);
  lu.writeSecondPart(1);
  int fd0 = fileIO->open(chunk0.c_str(),O_RDONLY);
  int fd1 = fileIO->open(chunk1.c_str(),O_RDONLY);
  BOOST_CHECK_EQUAL(true, -1 != fd0);
  BOOST_CHECK_EQUAL(true, -1 != fd1);
  BOOST_CHECK_EQUAL(lu.sizeAfterSecondWrite(0), fileIO->filesize(fd0));
  BOOST_CHECK_EQUAL(lu.sizeAfterSecondWrite(1), fileIO->filesize(fd1));
}

BOOST_AUTO_TEST_CASE( test_Counter_from_start )
{
  boost::shared_ptr<MockFileIO> fileIO = lu.getFileIO();
  StreamAvail sa(fileIO);  

  lu.writeFirstPart(0);
  // first part is 13 to inprogress, so 12 on disk, but the first 4 are not L1Accept, = 8
  BOOST_CHECK_EQUAL(8u, sa.countUpTo(XtcFileName(chunk0), 0, 100));
  BOOST_CHECK_EQUAL(8u, sa.countUpTo(XtcFileName(chunk0), 0, 100));

  lu.writeSecondPart(0);
  // now we get that 9th=dg 13 from the first part, and 6 more, but the last dg is disable, so should be 14
  BOOST_CHECK_EQUAL(14u, sa.countUpTo(XtcFileName(chunk0), 0, 100));
  BOOST_CHECK_EQUAL(14u, sa.countUpTo(XtcFileName(chunk0), 0, 100));

  lu.writeFirstPart(1);  // writes 13 more headers, but last is not on disk, and first is enable
  BOOST_CHECK_EQUAL(14u+11u, sa.countUpTo(XtcFileName(chunk0), 0, 100));
  BOOST_CHECK_EQUAL(11u, sa.countUpTo(XtcFileName(chunk1), 0, 100));
  
  lu.writeSecondPart(1);
  // get last from first part + 5 = 6
  BOOST_CHECK_EQUAL(11u+6u, sa.countUpTo(XtcFileName(chunk1), 0, 100));
  BOOST_CHECK_EQUAL(5u, sa.countUpTo(XtcFileName(chunk1), 0, 5));
}

BOOST_AUTO_TEST_CASE( test_Counter_mid )
{
  boost::shared_ptr<MockFileIO> fileIO = lu.getFileIO();
  StreamAvail sa(fileIO);  

  lu.writeFirstPart(0);
  lu.writeSecondPart(0);
  lu.writeFirstPart(1); 
  // There are 14 + 11 events at this point, but we test when we start at the 
  // second event that we get 24 back
  BOOST_CHECK_EQUAL(14u+10u, sa.countUpTo(XtcFileName(chunk0), 0x00018B60, 100));
}

BOOST_AUTO_TEST_SUITE_END()
