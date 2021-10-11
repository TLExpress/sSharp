#pragma once

#ifndef SCSFILEACCESS_HPP
#define SCSFILEACCESS_HPP

#ifdef SCSFILEACCESS
#define SCSFILEACCESS_DLL __declspec(dllexport)
#else
#define SCSFILEACCESS_DLL __declspec(dllimport)
#endif

#include <filesystem>
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

enum class scsSaveType{UncomFile =0, UncomFolder,ComFile,ComFolder};
typedef const uint8_t EntryMode;
EntryMode doNothing = 0x0, loadToMemory = 0x1, inflateStream = 0x2, identFile = 0x4, decryptSII = 0x8, getPathList = 0x10, dropContentAfterDone = 0x20;

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
	std::stringstream* content;
	std::string* fileName = nullptr;
	std::list<std::string>* pathList = nullptr;
public:
	SCSSentries(size_t entryPos, uint64_t hashcode, size_t filePos, scsSaveType saveType, uint32_t crc, uint32_t compressedSize, uint32_t uncompressedSize);
	SCSSentries(std::istream& stream, size_t entrypos, EntryMode accessMode);
	~SCSSentries();
	errno_t __stdcall generatePathList();
	errno_t __stdcall identFileType();
	errno_t __stdcall identFileType(std::istream& stream, std::streampos pos);
	errno_t __stdcall inflateContent();
	errno_t __stdcall inflateContent(std::istream& stream, std::streampos pos);
	errno_t __stdcall getContent(std::istream& stream);
	errno_t __stdcall getContent(std::istream& stream, std::streampos pos);
	errno_t __stdcall searchName(std::map<uint64_t,std::string>* map);
	errno_t __stdcall toAbsPath();
	bool __stdcall pathListAllAbs();
	bool __stdcall havecontent() { return _haveContent; };
	bool __stdcall haveFileName() { return _haveFileName; };
	bool __stdcall havePathList() { return _havePathList; };
	void __stdcall dropContent() { content->clear(); delete content; _haveContent = false; }
	std::list<std::string>* __stdcall getList() { return pathList; };
	std::stringstream* getContent() { return content; }
	scsFileType getFileType() { return fileType; }
	std::string getFileName() { return *fileName; }
	void setList(std::list<std::string>* list) { pathList = list; }
	size_t getFilePos() { return filePos; }
};

extern SCSFILEACCESS_DLL std::list<SCSSentries*>* __stdcall getEntryList(std::istream& stream,EntryMode accessMode);
extern SCSFILEACCESS_DLL std::list<SCSSentries*>* __stdcall getEntryListFromFile(std::string fileName, EntryMode accessMode);

extern SCSFILEACCESS_DLL std::list<std::string>* __stdcall entriesToList(std::list<SCSSentries*>& entryList);
extern SCSFILEACCESS_DLL std::list<std::string>* __stdcall streamToList(std::istream& stream);
extern SCSFILEACCESS_DLL std::list<std::string>* __stdcall fileToList(std::string fileName);

extern SCSFILEACCESS_DLL std::map<uint64_t, std::string>* __stdcall csvToMap(std::istream& stream);
extern SCSFILEACCESS_DLL std::map<uint64_t, std::string>* __stdcall fileToMap(std::string fileName);
extern SCSFILEACCESS_DLL std::map<uint64_t, std::string>* __stdcall folderToMap(std::string folderName);
extern SCSFILEACCESS_DLL std::map<uint64_t, std::string>* __stdcall getFolders(std::string root, std::string path);
extern SCSFILEACCESS_DLL std::map<uint64_t, std::string>* __stdcall listToMap(std::list<std::string> list);
extern SCSFILEACCESS_DLL std::map<uint64_t, std::string>* __stdcall LogToMap(std::string fileName);

extern SCSFILEACCESS_DLL std::stringstream* __stdcall listToStream(std::list<std::string>* list);
extern SCSFILEACCESS_DLL std::stringstream* __stdcall mapToCsv(std::map<uint64_t, std::string>* map);

extern SCSFILEACCESS_DLL errno_t __stdcall streamToFile(std::istream& stream, std::string fileName);
extern SCSFILEACCESS_DLL errno_t __stdcall listToFile(std::list<std::string>* list, std::string fileName);
extern SCSFILEACCESS_DLL errno_t __stdcall mapToFile(std::map<uint64_t, std::string>* map, std::string fileName);

extern SCSFILEACCESS_DLL size_t __stdcall getResolvedFolderCount(std::list<SCSSentries*>* entryList);
extern SCSFILEACCESS_DLL size_t __stdcall getResolvedDirCount(std::list<SCSSentries*>* entryList);
extern SCSFILEACCESS_DLL size_t __stdcall getResolvedFileCount(std::list<SCSSentries*>* entryList);

#endif