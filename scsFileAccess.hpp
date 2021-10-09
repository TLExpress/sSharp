#pragma once

#include <cstdint>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <list>
#include <map>
#include "scshash.hpp"
#include "scsFileAnalyzer.hpp"
#include "SII_Decrypt.hpp"
#include "zlib.h"

enum class scsSaveType{UncomFolder=0,UncomFile,ComFolder,ComFile};
typedef const uint8_t EntryMode;
EntryMode loadToMemory = 0x1, inflateStream = 0x2, identFile = 0x4, decryptSII = 0x8, getPathList = 0x10;

class SCSSentries
{
private:
	bool _haveContent=false;
	bool _haveFileName=false;
	bool _havePathList=false;
	bool _contentCompressed = false;
	size_t entryPos=0;
	uint64_t hashcode=0;
	size_t filePos=0;
	uint32_t compressedSize=0;
	uint32_t uncompressedSize=0;
	scsSaveType saveType= scsSaveType::UncomFile;
	uint32_t crc=0;
	EntryMode objStutus=0;
protected:
	scsFileType fileType= scsFileType::unknown;
	std::stringstream content;
	std::string* fileName = nullptr;
	std::list<std::string>* pathList = nullptr;
public:
	SCSSentries(size_t entryPos, uint64_t hashcode, size_t filePos, scsSaveType saveType, uint32_t crc, uint32_t compressedSize, uint32_t uncompressedSize);
	SCSSentries(std::istream& stream, size_t entrypos, EntryMode accessmode);
	~SCSSentries();
	errno_t generatePathList();
	errno_t	identFileType();
	errno_t	identFileType(std::istream& stream, std::streampos pos);
	errno_t inflateContent();
	errno_t inflateContent(std::istream& stream, std::streampos pos);
	errno_t getContent(std::istream& stream);
	errno_t getContent(std::istream& stream, std::streampos pos);
	errno_t searchName(std::map<uint64_t,std::string>* map);
	errno_t toAbsPath();
	bool pathListAllAbs();
	bool havecontent() { return _haveContent; };
	bool haveFileName() { return _haveFileName; };
	bool havePathList() { return _havePathList; };
};