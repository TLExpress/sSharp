/*------------------------------------------------------------------------------

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

------------------------------------------------------------------------------*/
#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdint>
#include <ctime>
#include <stdexcept>

struct S3nKHeader
{
	uint32_t sign;
	uint8_t unk;
	uint8_t seed;
};

const uint32_t S3nKSign = 0x014b6e33UL;
const size_t S3nKMSize = (size_t)6;// sizeof(S3nKHeader);
const size_t sizeofHeader = (size_t)6;

/*
	Key table entries were calculated from this formula:

	Key[i] = (((i shl 2) xor not i) shl 3) xor i
*/

const char S3nKKeyTable[] = {
	(char)0xF8, (char)0xD1, (char)0xAA, (char)0x83, (char)0x5C, (char)0x75, (char)0x0E, (char)0x27, (char)0xB0, (char)0x99, (char)0xE2, (char)0xCB, (char)0x14, (char)0x3D, (char)0x46, (char)0x6F,
	(char)0x68, (char)0x41, (char)0x3A, (char)0x13, (char)0xCC, (char)0xE5, (char)0x9E, (char)0xB7, (char)0x20, (char)0x09, (char)0x72, (char)0x5B, (char)0x84, (char)0xAD, (char)0xD6, (char)0xFF,
	(char)0xD8, (char)0xF1, (char)0x8A, (char)0xA3, (char)0x7C, (char)0x55, (char)0x2E, (char)0x07, (char)0x90, (char)0xB9, (char)0xC2, (char)0xEB, (char)0x34, (char)0x1D, (char)0x66, (char)0x4F,
	(char)0x48, (char)0x61, (char)0x1A, (char)0x33, (char)0xEC, (char)0xC5, (char)0xBE, (char)0x97, (char)0x00, (char)0x29, (char)0x52, (char)0x7B, (char)0xA4, (char)0x8D, (char)0xF6, (char)0xDF,
	(char)0xB8, (char)0x91, (char)0xEA, (char)0xC3, (char)0x1C, (char)0x35, (char)0x4E, (char)0x67, (char)0xF0, (char)0xD9, (char)0xA2, (char)0x8B, (char)0x54, (char)0x7D, (char)0x06, (char)0x2F,
	(char)0x28, (char)0x01, (char)0x7A, (char)0x53, (char)0x8C, (char)0xA5, (char)0xDE, (char)0xF7, (char)0x60, (char)0x49, (char)0x32, (char)0x1B, (char)0xC4, (char)0xED, (char)0x96, (char)0xBF,
	(char)0x98, (char)0xB1, (char)0xCA, (char)0xE3, (char)0x3C, (char)0x15, (char)0x6E, (char)0x47, (char)0xD0, (char)0xF9, (char)0x82, (char)0xAB, (char)0x74, (char)0x5D, (char)0x26, (char)0x0F,
	(char)0x08, (char)0x21, (char)0x5A, (char)0x73, (char)0xAC, (char)0x85, (char)0xFE, (char)0xD7, (char)0x40, (char)0x69, (char)0x12, (char)0x3B, (char)0xE4, (char)0xCD, (char)0xB6, (char)0x9F,
	(char)0x78, (char)0x51, (char)0x2A, (char)0x03, (char)0xDC, (char)0xF5, (char)0x8E, (char)0xA7, (char)0x30, (char)0x19, (char)0x62, (char)0x4B, (char)0x94, (char)0xBD, (char)0xC6, (char)0xEF,
	(char)0xE8, (char)0xC1, (char)0xBA, (char)0x93, (char)0x4C, (char)0x65, (char)0x1E, (char)0x37, (char)0xA0, (char)0x89, (char)0xF2, (char)0xDB, (char)0x04, (char)0x2D, (char)0x56, (char)0x7F,
	(char)0x58, (char)0x71, (char)0x0A, (char)0x23, (char)0xFC, (char)0xD5, (char)0xAE, (char)0x87, (char)0x10, (char)0x39, (char)0x42, (char)0x6B, (char)0xB4, (char)0x9D, (char)0xE6, (char)0xCF,
	(char)0xC8, (char)0xE1, (char)0x9A, (char)0xB3, (char)0x6C, (char)0x45, (char)0x3E, (char)0x17, (char)0x80, (char)0xA9, (char)0xD2, (char)0xFB, (char)0x24, (char)0x0D, (char)0x76, (char)0x5F,
	(char)0x38, (char)0x11, (char)0x6A, (char)0x43, (char)0x9C, (char)0xB5, (char)0xCE, (char)0xE7, (char)0x70, (char)0x59, (char)0x22, (char)0x0B, (char)0xD4, (char)0xFD, (char)0x86, (char)0xAF,
	(char)0xA8, (char)0x81, (char)0xFA, (char)0xD3, (char)0x0C, (char)0x25, (char)0x5E, (char)0x77, (char)0xE0, (char)0xC9, (char)0xB2, (char)0x9B, (char)0x44, (char)0x6D, (char)0x16, (char)0x3F,
	(char)0x18, (char)0x31, (char)0x4A, (char)0x63, (char)0xBC, (char)0x95, (char)0xEE, (char)0xC7, (char)0x50, (char)0x79, (char)0x02, (char)0x2B, (char)0xF4, (char)0xDD, (char)0xA6, (char)0x8F,
	(char)0x88, (char)0xA1, (char)0xDA, (char)0xF3, (char)0x2C, (char)0x05, (char)0x7E, (char)0x57, (char)0xC0, (char)0xE9, (char)0x92, (char)0xBB, (char)0x64, (char)0x4D, (char)0x36, (char)0x1F
};

/*==============================================================================
--------------------------------------------------------------------------------
							   TSII_3nK_Transcoder
--------------------------------------------------------------------------------
==============================================================================*/

/*==============================================================================
	TSII_3nK_Transcoder - declaration
==============================================================================*/

class S3nKTranscoder
{
	typedef void (S3nKTranscoder::* S3nKProcRoutine)(std::istream& input, std::ostream& output, bool rectifySize);
	typedef void (*S3nKProgressEvent)(S3nKTranscoder*, double);
	typedef void (*S3nKProgressCallback)(S3nKTranscoder*, double);
private:
	uint8_t fseed;
	S3nKProgressEvent fOnProgressEvent;
	S3nKProgressCallback fOnProgressCallback;
	char getKey(uint8_t index);
protected:
	virtual void doProgress(double progress);
	virtual void transcodeBuffer(char* buff, size_t size, int64_t seed);
	virtual void processFile(const std::string inFileName, const std::string outFileName, S3nKProcRoutine routine);
	virtual void processFileInMemory(const std::string inFileName, const std::string outFileName, S3nKProcRoutine routine);
public:
	S3nKTranscoder();
	virtual bool is3nKStream(std::istream& stream);
	virtual bool is3nKFile(std::string FileName);
	virtual void encodeStream(std::istream& input, std::ostream& output, bool invariantOutput);
	virtual void decodeStream(std::istream& input, std::ostream& output, bool invariantOutput);
	virtual void transcodeStream(std::istream& input, std::ostream& output, bool invariantOutput);
	virtual void decodeFile(std::string inFileName, std::string outFileName);
	virtual void encodeFile(std::string inFileName, std::string outFileName);
	virtual void transcodeFile(std::string inFileName, std::string outFileName);
	virtual void encodeFileInMemory(std::string inFileName, std::string outFileName);
	virtual void decodeFileInMemory(std::string inFileName, std::string outFileName);
	virtual void transcodeFileInMemory(std::string inFileName, std::string outFileName);
	__declspec(property(get = getKey)) char keys[];
	__declspec(property(get = fseed)) uint8_t seed;
	__declspec(property(get = fOnProgressEvent, put = fOnProgressEvent)) S3nKProgressEvent Onprogress;
	__declspec(property(get = fOnProgressEvent, put = fOnProgressEvent)) S3nKProgressEvent OnprogressEvent;
	__declspec(property(get = fOnProgressCallback, put = fOnProgressCallback)) S3nKProgressCallback OnprogressCallback;
};

const size_t S3nK_BufferSize = 16 * 1024; // 16KiB