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
#include <functional>
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

using std::stringstream;
using std::ofstream;
using std::function;
using std::vector;
using std::map;
using sfan::SCSPathList;
using sfan::_SCSPathList;
using sfan::FileType;
using sfan::Buff;
using sfan::SCSSException;

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
		const uint16_t doNothing = 0x0, loadToMemory = 0x1, inflateStream = 0x2, identFile = 0x4, foldersOnly = 0x8, decryptSII = 0x10, getPathList = 0x20, calculateCrc = 0x40, buildFolderContent = 0x80, identByFileName = 0x100, analyze = 0x200;
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
		explicit SCSEntry(SCSEntry& l);
		explicit SCSEntry(SCSEntry&& r) noexcept;
		~SCSEntry();

		SCSPathList generatePathList();
		FileType identFileType();
		FileType identByFileName();
		SCSContent inflateContent();
		SCSContent inflatedContent();
		SCSContent deflateContent(CompressedType compressed_type);
		SCSContent deflatedContent(CompressedType compressed_type);
		SCSContent decryptSII();
		SCSContent decryptedSII();
		SCSContent encryptSII();
		SCSContent encryptedSII();
		SCSContent receiveContent();
		SCSContent receivedContent();

		uint32_t toAbsPath();

		bool searchName(SCSDictionary map);
		bool pathListAllAbs();

		bool haveContent() { return _have_content; }
		bool haveFileName() { return _have_file_name; };
		bool havePathList() { return _have_path_list; };
		bool isDecrypted() { return _decrypted; }
		bool isPass() { return _pass; }

		void dropContent();
		void dropContentForce();
		void setHaveContent(bool have) { _have_content = have; }
		void setPathList(SCSPathList list) { _path_list = list; _have_path_list = true; }
		void setContent(SCSContent stream) { _content = stream; _have_content = true; } //incompleted
		void setFileType(FileType type) { _file_type = type; }
		void setFileName(string name) { _file_name = name; _hashcode = getHash(name); }
		void setSourceType(SourceType source_type) { _source_type = source_type; }
		void setUncompressedSize(uint32_t size) { _uncompressed_size = size; }
		void setOutputSize(uint32_t size) { _output_size = size; }
		void setCompressedType(CompressedType compressed_type) { _compressed_type = compressed_type; }

		uint32_t getCompressedSize() { return _compressed_size; }
		uint32_t getUncompressedSize() { return _uncompressed_size; }
		uint32_t getOutputSize() { return _output_size; }
		uint32_t getCrc() { return _crc; }
		uint64_t getHashcode() { return _hashcode; }
		string getFileName() { return _file_name; }
		CompressedType getCompressedType() { return _compressed_type; }
		FileType getFileType() { return _file_type; }
		SaveType getSaveType() { return _save_type; }
		SCSPathList getPathList() { return _path_list; }
		SCSContent getContent() { return _content; }
		SourceType getSourceType() { return _source_type; }
		string getSourcePath() { return _source_path; }

		uint32_t calcCrc();

		__declspec(property(get = haveContent, put = setHaveContent)) bool have_content;
		__declspec(property(get = haveFileName)) bool have_file_name;
		__declspec(property(get = havePathList)) bool have_path_list;
		__declspec(property(get = isDecrypted)) bool decrypted;
		__declspec(property(get = getCompressedType, put = setCompressedType)) CompressedType compressed_type;
		__declspec(property(get = getPathList, put = setPathList)) SCSPathList path_list;
		__declspec(property(get = getContent, put = setContent)) SCSContent content;
		__declspec(property(get = getFileName)) string file_name;
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
		__declspec(property(get = isPass)) bool pass;

		SCSEntry& operator=(SCSEntry& l);
		SCSEntry& operator=(SCSEntry&& r) noexcept;
		SCSEntry&& clone();
	};

	class _SCSEntryList;

	/*class pSCSEntry : public shared_ptr<SCSEntry>
	{
	public:
		shared_ptr::shared_ptr;
		pSCSEntry&& clone() { return make_shared<SCSEntry>(this->get()->clone()); }
	};*/
	typedef shared_ptr<SCSEntry> pSCSEntry;

	class _SCSEntryList : public vector<pSCSEntry>
	{
		function<int8_t(pSCSEntry, pSCSEntry)> default_comparator = [](pSCSEntry l, pSCSEntry r) 
		{
			if(l->hashcode < r->hashcode)
				return 1;
			if (l->hashcode > r->hashcode)
				return -1;
			return 0;
		};
	public:
		vector::vector;

		_SCSEntryList(_SCSEntryList& l) : vector(l) {}
		_SCSEntryList(_SCSEntryList&& r) noexcept : vector(move(r)) {}

		_SCSEntryList&& sublist(function<bool(pSCSEntry s)>filter);
		_SCSEntryList&& deep_clone();

		_SCSEntryList&& operator+(const pSCSEntry& l);
		_SCSEntryList&& operator-(const pSCSEntry& l);
		_SCSEntryList&& union_set(const pSCSEntry& l);
		_SCSEntryList&& cross_set(const pSCSEntry& l);
		_SCSEntryList&& union_set(const pSCSEntry& l, function<int8_t(pSCSEntry, pSCSEntry)> comparator);
		_SCSEntryList&& cross_set(const pSCSEntry& l, function<int8_t(pSCSEntry, pSCSEntry)> comparator);
		_SCSEntryList&& operator+(const _SCSEntryList& l);
		_SCSEntryList&& operator-(const _SCSEntryList& l);
		_SCSEntryList&& union_set(const _SCSEntryList& l);
		_SCSEntryList&& cross_set(const _SCSEntryList& l);
		_SCSEntryList&& union_set(const _SCSEntryList& l, function<int8_t(pSCSEntry, pSCSEntry)> comparator);
		_SCSEntryList&& cross_set(const _SCSEntryList& l, function<int8_t(pSCSEntry, pSCSEntry)> comparator);

		_SCSEntryList&& operator+(_SCSEntryList&& r);
		_SCSEntryList&& union_set(_SCSEntryList&& r);
		_SCSEntryList&& cross_set(_SCSEntryList&& r);

		_SCSEntryList&& operator-(function<bool(pSCSEntry s)>filter);

		_SCSEntryList& operator=(const pSCSEntry& l) { this->clear(); this->push_back(l); return *this; }
		_SCSEntryList& operator+=(const pSCSEntry& l) {	*this = *this + l; return *this; }
		_SCSEntryList& operator-=(const pSCSEntry& l) { *this = *this - l; return *this; }
		_SCSEntryList& operator=(const _SCSEntryList& l) { (vector)*this = l; return *this; }
		_SCSEntryList& operator+=(const _SCSEntryList& l) { *this = *this + l; return *this; }
		_SCSEntryList& operator-=(const _SCSEntryList& l) { *this = *this - l; return *this; }
		_SCSEntryList& operator-=(function<bool(pSCSEntry s)>filter) { *this = *this - filter; return *this; }
		_SCSEntryList& operator=(const _SCSEntryList&& r) noexcept { (vector)*this = move((vector)r); return *this; }
		_SCSEntryList& operator+=(_SCSEntryList&& r) { *this = *this + move(r); return *this; }
		void sort(function<bool(pSCSEntry l, pSCSEntry r)>comparator);
		void sort(SourceType file_type);
		void remove(const pSCSEntry& v) { erase(std::remove(this->begin(), this->end(), v), this->end()); }
		void remove(const uint64_t hash) { erase(std::remove_if(this->begin(), this->end(), [&hash](pSCSEntry s) {return s->hashcode == hash; }), this->end()); }
		void remove_if(function<bool(pSCSEntry)>comparator) { erase(std::remove_if(this->begin(), this->end(), comparator), this->end()); }
		void unique() { erase(std::unique(this->begin(), this->end(), [](pSCSEntry l, pSCSEntry r) {return l->hashcode == r->hashcode; }), this->end()); }
		void unique(function<bool(pSCSEntry, pSCSEntry)> comparator) { erase(std::unique(this->begin(), this->end(), comparator), this->end()); }
	};

	typedef shared_ptr<_SCSEntryList> SCSEntryList;

	extern SCSFILEACCESS_DLL SCSEntryList scssToEntries(string _file_name, uint16_t access_mode);
	extern SCSFILEACCESS_DLL SCSEntryList zipToEntries(string _file_name, uint16_t accessMode);
	extern SCSFILEACCESS_DLL SCSEntryList folderToEntries(string rootName, uint16_t accessMode);
	extern SCSFILEACCESS_DLL SCSEntryList fileToEntries(string rootName, uint16_t accessMode);

	extern SCSFILEACCESS_DLL SCSPathList getListFromFile(string _file_name);
	extern SCSFILEACCESS_DLL SCSPathList getListFromLog(string _file_name);
	extern SCSFILEACCESS_DLL SCSPathList convertMapToList(SCSDictionary map);
	extern SCSFILEACCESS_DLL SCSPathList entriesToList(SCSEntryList entryList);
	extern SCSFILEACCESS_DLL SCSPathList modFileToList(string _file_name, function<SCSEntryList(string, uint16_t)> method, uint16_t accessMode);

	extern SCSFILEACCESS_DLL SCSDictionary getMapFromFile(string _file_name);
	extern SCSFILEACCESS_DLL SCSDictionary getMapFromLog(string _file_name);
	extern SCSFILEACCESS_DLL SCSDictionary convertListToMap(SCSPathList list);
	extern SCSFILEACCESS_DLL SCSDictionary entriesToMap(SCSEntryList entryList);
	extern SCSFILEACCESS_DLL SCSDictionary modFileToMap(string _file_name, function<SCSEntryList(string, uint16_t)> method, uint16_t accessMode);

	extern SCSFILEACCESS_DLL errno_t saveListToFile(SCSPathList list, string _file_name);
	extern SCSFILEACCESS_DLL errno_t saveMapToFile(SCSDictionary map, string _file_name);

	extern SCSFILEACCESS_DLL errno_t entriesToScss(SCSEntryList entryList, string _file_name);
	extern SCSFILEACCESS_DLL errno_t entriesToScss(SCSEntryList entryList, string _file_name, function<bool(pSCSEntry)> filter);
	extern SCSFILEACCESS_DLL errno_t entriesToZip(SCSEntryList entryList, string _file_name);
	extern SCSFILEACCESS_DLL errno_t entriesToZip(SCSEntryList entryList, string _file_name, function<bool(pSCSEntry)> filter);
	extern SCSFILEACCESS_DLL errno_t entriesToFolder(SCSEntryList entryList, string _file_name);
	extern SCSFILEACCESS_DLL errno_t entriesToFolder(SCSEntryList entryList, string _file_name, function<bool(pSCSEntry)> filter);

	extern SCSFILEACCESS_DLL size_t getResolvedFolderCount(SCSEntryList entryList);
	extern SCSFILEACCESS_DLL size_t getResolvedDirCount(SCSEntryList entryList);
	extern SCSFILEACCESS_DLL size_t getResolvedFileCount(SCSEntryList entryList);

	extern SCSFILEACCESS_DLL size_t analyzeEntries(SCSEntryList entryList);
	extern SCSFILEACCESS_DLL size_t analyzeEntriesWithMap(SCSEntryList entryList, SCSDictionary map);

	extern SCSFILEACCESS_DLL uint32_t deflateEntryList(SCSEntryList entryList, function<bool(pSCSEntry)> filter, CompressedType compressed_type);
	extern SCSFILEACCESS_DLL uint32_t inflateEntryList(SCSEntryList entryList, function<bool(pSCSEntry)> filter);
	extern SCSFILEACCESS_DLL uint32_t decryptEntryList(SCSEntryList entryList, function<bool(pSCSEntry)> filter);
	extern SCSFILEACCESS_DLL uint32_t decryptEntryList(SCSEntryList entryList);
	extern SCSFILEACCESS_DLL uint32_t encryptEntryList(SCSEntryList entryList, function<bool(pSCSEntry)> filter);
	extern SCSFILEACCESS_DLL uint32_t receiveEntryListContents(SCSEntryList entryList, function<bool(pSCSEntry)> filter);
	extern SCSFILEACCESS_DLL uint32_t dropEntryListContents(SCSEntryList entryList, function<bool(pSCSEntry)> filter);

	extern SCSFILEACCESS_DLL void buildFolder(SCSEntryList entryList);
	extern SCSFILEACCESS_DLL void removeIndexes(SCSEntryList entry_list);

}

#endif