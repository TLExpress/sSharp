#pragma once

#ifndef SCSFILEACCESS_HPP_
#define SCSFILEACCESS_HPP_

#ifdef _WINDLL
#ifdef SCSFILEACCESS
#define SCSFILEACCESS_DLL __declspec(dllexport)
#else
#define SCSFILEACCESS_DLL __declspec(dllimport)
#endif
#define LOAD_SIIDECRYPT
#else
#define SCSFILEACCESS_DLL
#endif

#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include "scsFileAnalyzer.hpp"
#include "scshash.hpp"
#include "3nK.hpp"
#include "zlib.h"

#ifdef LOAD_SIIDECRYPT
#include "SII_Decrypt.hpp"
#endif

namespace sfan = scsFileAnalyzer;
namespace stdfs = std::filesystem;

using std::ifstream;
using std::ofstream;
using std::stringstream;
using std::map;
using std::vector;
using sfan::SCSPathList;
using sfan::_SCSPathList;
using sfan::FileType;
using sfan::Buff;


namespace scsFileAccess
{
	enum class SaveType { UncomFile = 0, UncomFolder, ComFile, ComFolder };
	enum class SourceType { SCS = 0, ZIP, File, NoSource };
	enum class CompressedType { SCS = 0, ZIP, Uncompressed };

	typedef map<uint64_t, string> _SCSDictionary;
	typedef shared_ptr<_SCSDictionary> SCSDictionary;
	typedef shared_ptr<stringstream> SCSContent;
	namespace EntryMode
	{
		const uint16_t doNothing = 0x0, loadToMemory = 0x1, inflateStream = 0x2, identFile = 0x4, foldersOnly = 0x8, decryptSII = 0x10, getPathList = 0x20, calculateCrc = 0x40, buildFolderContent = 0x80, identByFileName = 0x100;
	}

	class SCSFILEACCESS_DLL SCSEntry
	{

	private:

		bool _have_content = false;
		bool _have_file_name = false;
		bool _have_path_list = false;
		bool _pass = false;
		size_t _entry_pos = 0;
		uint64_t _hashcode = 0;
		size_t _file_pos = 0;
		uint32_t _compressed_size = 0;
		uint32_t _uncompressed_size = 0;
		uint32_t _output_size = 0;
		CompressedType _compressed_type = CompressedType::Uncompressed;
		bool _decrypted = false;
		SaveType _save_type = SaveType::UncomFile;
		uint32_t _crc = 0;
		SourceType _source_type = SourceType::SCS;
		string _source_path = "";
		FileType _file_type = FileType::otherFiles;
		SCSContent _content;
		string _file_name = "";
		SCSPathList _path_list = nullptr;

	public:

		SCSEntry(bool have_content, bool have_file_name, bool have_path_list, bool pass, size_t entry_pos, uint64_t hashcode, size_t file_pos, uint32_t compressed_size, uint32_t uncompressed_size, CompressedType compressed_type,
			bool decrypted, SaveType save_type, uint32_t crc, SourceType source_type, string sourcePath, FileType file_type, SCSContent content, string file_name, SCSPathList path_list);
		SCSEntry(string sourcePath, size_t entryPos, uint16_t accessMode, SourceType source);
		SCSEntry(string filePath, string rootPath, uint16_t accessMode);
		~SCSEntry();

		void __stdcall generatePathList();
		void __stdcall identFileType();
		void __stdcall identByFileName();
		SCSContent __stdcall inflateContent();
		SCSContent __stdcall inflatedContent();
		SCSContent __stdcall deflateContent(CompressedType compressed_type);
		SCSContent __stdcall deflatedContent(CompressedType compressed_type);
		SCSContent __stdcall decryptSII();
		SCSContent __stdcall decryptedSII();
		SCSContent __stdcall encryptSII();
		SCSContent __stdcall encryptedSII();
		SCSContent __stdcall receiveContent();
		SCSContent __stdcall receivedContent();

		uint32_t __stdcall toAbsPath();

		bool __stdcall searchName(SCSDictionary map);
		bool __stdcall pathListAllAbs();

		bool __stdcall haveContent() { return _have_content; }
		bool __stdcall haveFileName() { return _have_file_name; };
		bool __stdcall havePathList() { return _have_path_list; };
		bool __stdcall isDecrypted() { return _decrypted; }
		bool __stdcall isPass() { return _pass; }

		void __stdcall dropContent();
		void __stdcall dropContentForce();
		void __stdcall setPathList(SCSPathList list) { _path_list = list; _have_path_list = true; }
		void __stdcall setContent(SCSContent stream) { _content = stream; _have_content = true; } //incompleted
		void __stdcall setFileType(FileType type) { _file_type = type; }
		void __stdcall setFileName(string name) { _file_name = name; _hashcode = getHash(name); }
		void __stdcall setSourceType(SourceType source_type) { _source_type = source_type; }
		void __stdcall setUncompressedSize(uint32_t size) { _uncompressed_size = size; }
		void __stdcall setOutputSize(uint32_t size) { _output_size = size; }
		void __stdcall setCompressedType(CompressedType compressed_type) { _compressed_type = compressed_type; }

		uint32_t __stdcall getCompressedSize() { return _compressed_size; }
		uint32_t __stdcall getUncompressedSize() { return _uncompressed_size; }
		uint32_t __stdcall getOutputSize() { return _output_size; }
		uint32_t __stdcall getCrc() { return _crc; }
		uint64_t __stdcall getHashcode() { return _hashcode; }
		string __stdcall getFileName() { return _file_name; }
		CompressedType __stdcall getCompressedType() { return _compressed_type; }
		FileType __stdcall getFileType() { return _file_type; }
		SaveType __stdcall getSaveType() { return _save_type; }
		SCSPathList __stdcall getPathList() { return _path_list; }
		SCSContent __stdcall getContent() { return _content; }
		SourceType __stdcall getSourceType() { return _source_type; }
		string __stdcall getSourcePath() { return _source_path; }


		uint32_t __stdcall calcCrc();

		__declspec(property(get = haveContent)) bool have_content;
		__declspec(property(get = haveFileName)) bool have_file_name;
		__declspec(property(get = havePathList)) bool have_path_list;
		__declspec(property(get = isDecrypted)) bool decrypted;
		__declspec(property(get = getCompressedType, put = setCompressedType)) CompressedType compressed_type;
		__declspec(property(get = getPathList, put = setPathList)) SCSPathList path_list;
		__declspec(property(get = getContent, put = setContent)) SCSContent content;
		__declspec(property(get = getFileName, put = _file_name)) string file_name;
		__declspec(property(get = getSourcePath)) string source_path;
		__declspec(property(get = getFileType, put = setFileType)) FileType file_type;
		__declspec(property(get = getSourceType, put = setSourceType)) SourceType source_type;
		__declspec(property(get = getSaveType)) SaveType save_type;
		__declspec(property(get = getHashcode)) uint64_t hashcode;
		__declspec(property(get = getCrc)) uint32_t crc;
		__declspec(property(get = getUncompressedSize, put = setUncompressedSize)) uint32_t uncompressed_size;
		__declspec(property(get = getCompressedSize)) uint32_t compressed_size;
		__declspec(property(get = getOutputSize, put = setOutputSize)) uint32_t output_size;
		__declspec(property(get = _file_pos)) size_t file_pos;
		__declspec(property(get = isPass, put =_pass)) bool pass;

	};

	typedef shared_ptr<SCSEntry> pSCSEntry;
	typedef vector<shared_ptr<SCSEntry>> _SCSEntryList;
	typedef shared_ptr<_SCSEntryList> SCSEntryList;

	extern SCSFILEACCESS_DLL SCSEntryList __stdcall scssToEntries(string _file_name, uint16_t access_mode);
	extern SCSFILEACCESS_DLL SCSEntryList __stdcall zipToEntries(string _file_name, uint16_t accessMode);
	extern SCSFILEACCESS_DLL SCSEntryList __stdcall folderToEntries(string rootName, uint16_t accessMode);
	extern SCSFILEACCESS_DLL SCSEntryList __stdcall fileToEntries(string rootName, uint16_t accessMode);

	extern SCSFILEACCESS_DLL SCSPathList __stdcall getListFromFile(string _file_name);
	extern SCSFILEACCESS_DLL SCSPathList __stdcall getListFromLog(string _file_name);
	extern SCSFILEACCESS_DLL SCSPathList __stdcall convertMapToList(SCSDictionary map);
	extern SCSFILEACCESS_DLL SCSPathList __stdcall entriesToList(SCSEntryList entryList);
	extern SCSFILEACCESS_DLL SCSPathList __stdcall modFileToList(string _file_name, SCSEntryList (*method)(string, uint16_t), uint16_t accessMode);

	extern SCSFILEACCESS_DLL SCSDictionary __stdcall getMapFromFile(string _file_name);
	extern SCSFILEACCESS_DLL SCSDictionary __stdcall getMapFromLog(string _file_name);
	extern SCSFILEACCESS_DLL SCSDictionary __stdcall convertListToMap(SCSPathList list);
	extern SCSFILEACCESS_DLL SCSDictionary __stdcall entriesToMap(SCSEntryList entryList);
	extern SCSFILEACCESS_DLL SCSDictionary __stdcall modFileToMap(string _file_name, SCSEntryList (*method)(string, uint16_t), uint16_t accessMode);

	extern SCSFILEACCESS_DLL errno_t __stdcall saveListToFile(SCSPathList list, string _file_name);
	extern SCSFILEACCESS_DLL errno_t __stdcall saveMapToFile(SCSDictionary map, string _file_name);

	extern SCSFILEACCESS_DLL errno_t __stdcall entryToScss(SCSEntryList entryList, string _file_name);
	extern SCSFILEACCESS_DLL errno_t __stdcall entryToZip(SCSEntryList entryList, string _file_name);
	extern SCSFILEACCESS_DLL errno_t __stdcall entryToFolder(SCSEntryList entryList, string _file_name);

	extern SCSFILEACCESS_DLL size_t __stdcall getResolvedFolderCount(SCSEntryList entryList);
	extern SCSFILEACCESS_DLL size_t __stdcall getResolvedDirCount(SCSEntryList entryList);
	extern SCSFILEACCESS_DLL size_t __stdcall getResolvedFileCount(SCSEntryList entryList);

	extern SCSFILEACCESS_DLL size_t __stdcall analyzeEntries(SCSEntryList entryList);
	extern SCSFILEACCESS_DLL size_t __stdcall analyzeEntriesWithMap(SCSEntryList entryList, SCSDictionary map);

	extern SCSFILEACCESS_DLL uint32_t __stdcall deflateEntryList(SCSEntryList entryList, bool (*filter)(pSCSEntry), CompressedType compressed_type);
	extern SCSFILEACCESS_DLL uint32_t __stdcall inflateEntryList(SCSEntryList entryList, bool (*filter)(pSCSEntry));
	extern SCSFILEACCESS_DLL uint32_t __stdcall decryptEntryList(SCSEntryList entryList, bool (*filter)(pSCSEntry));
	extern SCSFILEACCESS_DLL uint32_t __stdcall encryptEntryList(SCSEntryList entryList, bool (*filter)(pSCSEntry));
	extern SCSFILEACCESS_DLL uint32_t __stdcall receiveEntryListContents(SCSEntryList entryList, bool (*filter)(pSCSEntry));
	extern SCSFILEACCESS_DLL uint32_t __stdcall dropEntryListContents(SCSEntryList entryList, bool (*filter)(pSCSEntry));

	extern SCSFILEACCESS_DLL void __stdcall buildFolder(SCSEntryList entryList);

}

#endif