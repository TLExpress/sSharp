#pragma once

#ifndef SCSFILEACCESS_HPP_
#define SCSFILEACCESS_HPP_

#ifdef SCSFILEACCESS
#define SCSFILEACCESS_DLL __declspec(dllexport)
#else
#define SCSFILEACCESS_DLL __declspec(dllimport)
#endif

#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <string>
#include <list>
#include <map>
#include "scsFileAnalyzer.hpp"
#include "scshash.hpp"
#include "3nK.hpp"
#include "zlib.h"

#ifdef LOAD_SIIDECRYPT
#include "SII_Decrypt.hpp"
#endif

enum class scsSaveType{UncomFile =0, UncomFolder,ComFile,ComFolder};
enum class sourceType{SCS=0,ZIP=1,FILE=2};
typedef const uint8_t EntryMode;
EntryMode doNothing = 0x0, loadToMemory = 0x1, inflateStream = 0x2, identFile = 0x4, decryptSII = 0x8, getPathList = 0x10, dropContentAfterDone = 0x20;

class SCSFILEACCESS_DLL SCSSentries
{

private:

	bool _haveContent=false;
	bool _haveFileName=false;
	bool _havePathList=false;
	bool _contentCompressed = false;
	size_t entryPos = 0;
	uint64_t hashcode = 0;
	size_t filePos = 0;
	uint32_t compressedSize = 0;
	uint32_t uncompressedSize= 0;
	uint32_t undecryptedSize = 0;
	scsSaveType saveType= scsSaveType::UncomFile;
	uint32_t crc=0;
	EntryMode objStutus = 0;
	sourceType source = sourceType::SCS;

protected:

	scsFileType fileType = scsFileType::unknown;
	std::stringstream* content;
	std::string* fileName = nullptr;
	std::list<std::string>* pathList = nullptr;

public:

	SCSSentries(size_t entryPos, uint64_t hashcode, size_t filePos, scsSaveType saveType, uint32_t crc, uint32_t compressedSize, uint32_t uncompressedSize, sourceType source);
	SCSSentries(std::istream& stream, size_t entrypos, EntryMode accessMode);
	~SCSSentries();

	errno_t __stdcall generatePathList();
	errno_t __stdcall identFileType(std::istream& stream, size_t pos, size_t size);
	errno_t __stdcall identFileType();
	std::stringstream* __stdcall inflateContent();
	std::stringstream* __stdcall inflateContent(std::istream& stream, std::streampos pos);
	std::stringstream* __stdcall decryptSIIStream();
	std::stringstream* __stdcall decryptSIIStream(std::istream& stream, size_t pos, size_t size);
	std::stringstream* __stdcall getContent(std::istream& stream);
	std::stringstream* __stdcall getContent(std::istream& stream, std::streampos pos);
	errno_t __stdcall searchName(std::map<uint64_t,std::string>* map);
	errno_t __stdcall toAbsPath();

	bool __stdcall pathListAllAbs();
	bool __stdcall havecontent() { return _haveContent; };
	bool __stdcall haveFileName() { return _haveFileName; };
	bool __stdcall havePathList() { return _havePathList; };
	bool __stdcall isCompressed() { return _contentCompressed; }

	void __stdcall dropContent() { content->clear(); delete content; _haveContent = false; uncompressedSize = undecryptedSize; }
	void __stdcall setList(std::list<std::string>* list) { pathList = list; }
	void __stdcall setContent(std::stringstream* stream) { content = stream; _haveContent = true; }
	void __stdcall setFileType(scsFileType type) { fileType = type; }

	std::list<std::string>* __stdcall getList() { return pathList; };
	std::stringstream* __stdcall getContent() { return content; }
	std::string __stdcall getFileName() { return *fileName; }
	scsFileType __stdcall getFileType() { return fileType; }
	uint64_t __stdcall getHashCode() { return hashcode; }
	uint32_t __stdcall getCrc() { return crc; }
	uint32_t __stdcall getUncompressedSize() { return uncompressedSize; }
	uint32_t __stdcall getCompressedSize() { return compressedSize; }
	size_t __stdcall getFilePos() { return filePos; }
};

extern SCSFILEACCESS_DLL std::list<SCSSentries*>* __stdcall scssToEntries(std::istream& stream, EntryMode accessMode);
extern SCSFILEACCESS_DLL std::list<SCSSentries*>* __stdcall scssToEntries(std::string fileName, EntryMode accessMode);
extern SCSFILEACCESS_DLL std::list<SCSSentries*>* __stdcall zipToEntries(std::istream& stream, EntryMode accessMode);
extern SCSFILEACCESS_DLL std::list<SCSSentries*>* __stdcall zipToEntries(std::string fileName, EntryMode accessMode);
extern SCSFILEACCESS_DLL std::list<SCSSentries*>* __stdcall fileToEntries(std::string rootName, EntryMode accessMode);

extern SCSFILEACCESS_DLL std::list<std::string>* __stdcall entriesToList(std::list<SCSSentries*>& entryList);
extern SCSFILEACCESS_DLL std::list<std::string>* __stdcall scssToList(std::istream& stream, EntryMode accessMode);
extern SCSFILEACCESS_DLL std::list<std::string>* __stdcall scssToList(std::string fileName, EntryMode accessMode);
extern SCSFILEACCESS_DLL std::list<std::string>* __stdcall zipToList(std::string fileName);
extern SCSFILEACCESS_DLL std::list<std::string>* __stdcall zipToList(std::istream& stream);
extern SCSFILEACCESS_DLL std::list<std::string>* __stdcall fileToList(std::istream& stream);
extern SCSFILEACCESS_DLL std::list<std::string>* __stdcall fileToList(std::string fileName);
extern SCSFILEACCESS_DLL std::list<std::string>* __stdcall mapToList(std::map<uint64_t, std::string>*map);

extern SCSFILEACCESS_DLL std::map<uint64_t, std::string>* __stdcall scssToMap(std::string fileName, EntryMode accessMode);
extern SCSFILEACCESS_DLL std::map<uint64_t, std::string>* __stdcall scssToMap(std::istream& stream);
extern SCSFILEACCESS_DLL std::map<uint64_t, std::string>* __stdcall zipToMap(std::string fileName);
extern SCSFILEACCESS_DLL std::map<uint64_t, std::string>* __stdcall zipToMap(std::istream& stream);
extern SCSFILEACCESS_DLL std::map<uint64_t, std::string>* __stdcall fileToMap(std::istream& stream);
extern SCSFILEACCESS_DLL std::map<uint64_t, std::string>* __stdcall fileToMap(std::string fileName);
extern SCSFILEACCESS_DLL std::map<uint64_t, std::string>* __stdcall folderToMap(std::string folderName);
extern SCSFILEACCESS_DLL std::map<uint64_t, std::string>* __stdcall folderToMap(std::string root, std::string path);
extern SCSFILEACCESS_DLL std::map<uint64_t, std::string>* __stdcall listToMap(std::list<std::string> list);
extern SCSFILEACCESS_DLL std::map<uint64_t, std::string>* __stdcall LogToMap(std::string fileName);

extern SCSFILEACCESS_DLL std::stringstream* __stdcall listToStream(std::list<std::string>* list);
extern SCSFILEACCESS_DLL std::stringstream* __stdcall mapToStream(std::map<uint64_t, std::string>* map);

extern SCSFILEACCESS_DLL errno_t __stdcall streamToFile(std::istream& stream, std::string fileName);
extern SCSFILEACCESS_DLL errno_t __stdcall listToFile(std::list<std::string>* list, std::string fileName);
extern SCSFILEACCESS_DLL errno_t __stdcall mapToFile(std::map<uint64_t, std::string>* map, std::string fileName);

extern SCSFILEACCESS_DLL errno_t __stdcall entryToScss(std::list<SCSSentries*>* entryList, std::string fileName);
extern SCSFILEACCESS_DLL errno_t __stdcall entryToZip(std::list<SCSSentries*>* entryList, std::string fileName);
extern SCSFILEACCESS_DLL errno_t __stdcall entryToFile(std::list<SCSSentries*>* entryList, std::string fileName, std::string sourceName);

extern SCSFILEACCESS_DLL size_t __stdcall getResolvedFolderCount(std::list<SCSSentries*>* entryList);
extern SCSFILEACCESS_DLL size_t __stdcall getResolvedDirCount(std::list<SCSSentries*>* entryList);
extern SCSFILEACCESS_DLL size_t __stdcall getResolvedFileCount(std::list<SCSSentries*>* entryList);

#endif