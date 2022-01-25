#include "scsFileAccess.hpp"

SCSSentries::SCSSentries(size_t entryPos, uint64_t hashcode, size_t filePos, scsSaveType saveType, uint32_t crc, uint32_t compressedSize, uint32_t uncompressedSize, sourceType source)
{
	this->entryPos = entryPos;
	this->hashcode = hashcode;
	this->saveType = saveType;
	this->crc = crc;
	this->compressedSize = compressedSize;
	this-> undecryptedSize = this->uncompressedSize = uncompressedSize;
	this->content = nullptr;
	this->source = source;
}

SCSSentries::SCSSentries(std::istream& stream, size_t entryPos, EntryMode accessMode)
{
	if (!stream)throw("StreamIOError");
	this->entryPos = entryPos;
	stream.seekg(entryPos, std::ios::beg);
	stream.read((char*)&(this->hashcode), sizeof(uint64_t));
	stream.read((char*)&(this->filePos), sizeof(size_t));
	if (sizeof(size_t) < 8)stream.seekg(8 - sizeof(size_t), std::ios::cur);
	uint32_t tempStype;
	stream.read((char*)&tempStype, sizeof(uint32_t));
	this->saveType = (scsSaveType)(tempStype&0x3);
	if (((uint8_t)saveType & 2) == 2)
		this->_contentCompressed = true;
	stream.read((char*)&(this->crc), sizeof(uint32_t));
	stream.read((char*)&(this->uncompressedSize), sizeof(uint32_t));
	this->undecryptedSize = this->uncompressedSize;
	stream.read((char*)&(this->compressedSize), sizeof(uint32_t));
	this->content = nullptr;
	if (accessMode & loadToMemory)
		getContent(stream);
	if (accessMode & inflateStream)
		if (((uint8_t)saveType & 2) == 2)
			inflateContent();
	if (accessMode & identFile)
	{
		if (!havecontent() || _contentCompressed)throw("Contents is not ready");
		identFileType();
	}
	scsFileType tempType = this->fileType;
	if (accessMode & decryptSII)
	{
		if (!havecontent()|| _contentCompressed)throw("Contents is not ready");
		this->decryptSIIStream();
	}
	if (accessMode & getPathList)
	{
		if (!havecontent() || _contentCompressed)throw("Contents is not ready");
		this->generatePathList();
	}
	if (accessMode & dropContentAfterDone)
	{
		this->dropContent();
		this->fileType = tempType;
	}
}

SCSSentries::~SCSSentries()
{
	delete pathList;
	content->clear();
	delete[] content;
}

errno_t __stdcall SCSSentries::generatePathList()
{
	if (!havecontent() || _contentCompressed)return -1;
	if (_havePathList)return 0;
	if (fileType == scsFileType::folder)
		pathList = extractFolderToList(*content);
	else
		pathList = extractToList(*this->content);
	if (this->pathList != nullptr)
		this->_havePathList = true;
	return 0;
}

errno_t __stdcall SCSSentries::identFileType(std::istream& stream, size_t pos, size_t size)
{
	if (saveType == scsSaveType::ComFolder || saveType == scsSaveType::UncomFolder)
	{
		fileType = scsFileType::folder;
		return 0;
	}
	if (!havecontent() || _contentCompressed)return -1;
	fileType = scsAnalyzeStream(stream, pos, size);
	return 0;
}

errno_t __stdcall SCSSentries::identFileType()
{
	if (saveType == scsSaveType::ComFolder || saveType == scsSaveType::UncomFolder)
	{
		fileType = scsFileType::folder;
		return 0;
	}
	if (!havecontent() || _contentCompressed)return -1;
	content->seekg(0, std::ios::beg);
	fileType = scsAnalyzeStream(*content);
	content->seekg(0, std::ios::beg);
	return 0;
}

std::stringstream* __stdcall SCSSentries::inflateContent()
{
	if (!havecontent())return nullptr;
	if (_contentCompressed == true)
		inflateContent(*content, 0);
	return content;
}

std::stringstream* __stdcall SCSSentries::inflateContent(std::istream& stream, std::streampos pos)
{
	if (!stream)return content;
	if (_contentCompressed == false)return 0;
	char* buff = new char[compressedSize];
		stream.seekg(pos, std::ios::beg);
		stream.read(buff, compressedSize);
	size_t bsize = compressBound(uncompressedSize);
	char* ubuff = new char[bsize];
	uncompress3((Bytef*)ubuff, (uLongf*)&bsize, (Bytef*)buff, (uLong)compressedSize,source ==sourceType::SCS?15:-15);
	delete[] buff;
	content->seekg(0, std::ios::beg);
	content->clear();
	delete[] content;
	content = new std::stringstream(std::ios::in | std::ios::out | std::ios::binary);
	content->seekg(0, std::ios::beg);
	content->write(ubuff, bsize);
	delete[] ubuff;
	_contentCompressed = false;
	return content;
}

std::stringstream* __stdcall SCSSentries::decryptSIIStream()
{
	if (!_haveContent || fileType != scsFileType::siiBinary && fileType != scsFileType::siiEncrypted && fileType != scsFileType::sii3nK)
		return content;
	S3nKTranscoder transcoder;
	if (transcoder.is3nKStream(*content))
	{
		std::stringstream* decrypted = new std::stringstream(std::ios::in | std::ios::out | std::ios::binary);
		transcoder.decodeStream(*content, *decrypted, false);
		delete content;
		content = decrypted;
		content->seekg(0, std::ios::end);
		uncompressedSize -=6 ;
		content->seekg(0, std::ios::beg);
		if(scsAnalyzeStream(*decrypted)==scsFileType::sii)
			return content;
	}
#ifdef LOAD_SIIDECRYPT
	size_t decryptSize = (size_t)uncompressedSize * 64;
	char* in = new char[uncompressedSize];
	char* out = new char[decryptSize];
	this->content->seekg(0, std::ios::beg);
	this->content->read(in, uncompressedSize);
	uint32_t ret = DecryptAndDecodeMemory(in, uncompressedSize, out, &decryptSize);
	delete[] in;
	this->content->clear();
	delete[] this->content;
	this->content = new std::stringstream(std::ios::in | std::ios::out | std::ios::binary);
	this->content->seekg(0, std::ios::beg);
	this->content->write(out, decryptSize);
	delete[] out;
	this->content->seekg(0, std::ios::end);
	uncompressedSize = (uint32_t)this->content->tellg();
	this->content->seekg(0, std::ios::beg);
	this->fileType = scsFileType::sii;
	return content;
#else
	return nullptr;
#endif

}

std::stringstream* __stdcall SCSSentries::decryptSIIStream(std::istream& stream, size_t pos, size_t size)
{
	if (fileType != scsFileType::siiBinary && fileType != scsFileType::siiEncrypted && fileType != scsFileType::sii3nK)
		return content;
	S3nKTranscoder transcoder;
	if (transcoder.is3nKStream(stream,pos,size))
	{
		char* buff = new char[size];
		stream.seekg(pos, std::ios::beg);
		stream.read(buff, size);
		std::stringstream* encrypted = new std::stringstream(std::ios::in | std::ios::out | std::ios::binary);
		encrypted->write(buff, size);
		delete[] buff;
		std::stringstream* decrypted = new std::stringstream(std::ios::in | std::ios::out | std::ios::binary);
		transcoder.decodeStream(*encrypted, *decrypted, false);
		delete content;
		content = decrypted;
		content->seekg(0, std::ios::end);
		uncompressedSize -= 6;
		content->seekg(0, std::ios::beg);
		if (scsAnalyzeStream(*decrypted) == scsFileType::sii)
			return content;
	}
#ifdef LOAD_SIIDECRYPT
	size_t decryptSize = (size_t)size * 64;
	char* in = new char[size];
	char* out = new char[decryptSize];
	stream.seekg(pos, std::ios::beg);
	stream.read(in, uncompressedSize);
	stream.seekg(pos, std::ios::beg);
	uint32_t ret = DecryptAndDecodeMemory(in, size, out, &decryptSize);
	delete[] in;
	this->content->clear();
	delete[] this->content;
	this->content = new std::stringstream(std::ios::in | std::ios::out | std::ios::binary);
	this->content->seekg(0, std::ios::beg);
	this->content->write(out, decryptSize);
	delete[] out;
	this->content->seekg(0, std::ios::end);
	uncompressedSize = (uint32_t)this->content->tellg();
	this->content->seekg(0, std::ios::beg);
	this->fileType = scsFileType::sii;
	return content;
#else
	return nullptr;
#endif

}

std::stringstream* __stdcall SCSSentries::getContent(std::istream& stream)
{
	return getContent(stream, filePos);
}

std::stringstream* __stdcall SCSSentries::getContent(std::istream& stream, std::streampos pos)
{
	stream.seekg(pos, std::ios::beg);
	content = new std::stringstream(std::ios::in | std::ios::out | std::ios::binary);
	char* buff = new char[compressedSize];
	stream.read(buff, compressedSize);
	content->write(buff, compressedSize);
	if (((uint8_t)saveType & 2) == 2)
		this->_contentCompressed = true;
	delete[] buff;
	_haveContent = true;
	return content;
}

errno_t __stdcall SCSSentries::searchName(std::map<uint64_t, std::string>* map)
{
	if (map == nullptr)return -1;
	auto itr = map->find(hashcode);
	if (itr != map->end())
	{
		fileName = new std::string(itr->second);
		_haveFileName = true;
		return 0;
	}
	return 1;
}

errno_t __stdcall SCSSentries::toAbsPath()
{
	if (fileName == nullptr||pathList == nullptr)return -1;
	for (auto itr : *pathList)
	{
		if (itr.at(0) == '~' || itr.at(0) == '*')
		{
			if (fileType == scsFileType::folder)
				itr = *fileName + (fileName->size()==0?"":"/") + itr.substr(1);
			else
				itr = (fileName->find('/') == std::string::npos) ? itr.substr(1) : (fileName->substr(0,fileName->find_last_of('/') + 1) + itr.substr(1));
			pathList->push_back(itr);
		}
	}
	pathList->sort();
	pathList->remove_if([](std::string s) {return s.at(0) == '~' || s.at(0) == '*'; });
	return 0;
}

bool __stdcall SCSSentries::pathListAllAbs()
{
	if (pathList == nullptr)return true;
	for (auto itr : *pathList)
		if (itr.at(0) == '~' || itr.at(0) == '*')
			return false;
	return true;
}

std::list<SCSSentries*>* __stdcall scssToEntries(std::istream& stream, EntryMode accessMode)
{
	if (!stream)throw("IO error");
	std::list<SCSSentries*>* entryList = new std::list<SCSSentries*>();
	uint32_t sign;
	uint32_t entryCount;

	stream.seekg(0, std::ios::beg);
	stream.read((char*)&sign,4);
	if (sign != 0x23534353)throw("Invalid file");

	stream.seekg(0xC, std::ios::beg);
	stream.read((char*)&entryCount, sizeof(entryCount));

	stream.seekg(0x1000, std::ios::beg);
	std::streampos pos = stream.tellg();
	for (uint32_t counter = 0; counter < entryCount; counter++)
	{
		entryList->push_back(new SCSSentries(stream, pos, accessMode));
		pos += 0x20;
	}
	return entryList;
}

std::list<SCSSentries*>* __stdcall scssToEntries(std::string fileName, EntryMode accessMode)
{
	std::ifstream stream(fileName, std::ios::in | std::ios::binary);
	auto returnValue = scssToEntries(stream, accessMode);
	stream.close();
	return returnValue;
}

std::list<std::string>* __stdcall entriesToList(std::list<SCSSentries*>& entryList)
{
	std::list<std::string>* pathList = new std::list<std::string>();
	for (auto itr : entryList)
	{
		if (!itr->havePathList())itr->generatePathList();
		if (itr->havePathList())
		{
			std::list<std::string> teplst(*itr->getList());
			teplst.remove_if([](std::string s) {return s.at(0) == '~' || s.at(0) == '*'; });
			pathList->merge(teplst);
			pathList->unique();
		}
	}
	return pathList;
}

std::list<std::string>* __stdcall fileToList(std::istream& stream)
{
	if (!stream)throw("IO error");
	std::list<std::string>* list = new std::list<std::string>();
	std::string str;
	stream.seekg(0, std::ios::beg);
	while (!stream.eof())
	{
		std::getline(stream, str);
		list->push_back(str);
	}
	list->sort();
	list->unique();
	return list;
}

std::list<std::string>* __stdcall fileToList(std::string fileName)
{
	std::ifstream stream(fileName, std::ios::in);
	auto ret = fileToList(stream);
	stream.close();
	return ret;
}

std::map<uint64_t, std::string>* __stdcall fileToMap(std::istream& stream)
{
	if (!stream)throw("IO error");
	std::map<uint64_t, std::string>* map = new std::map<uint64_t, std::string>();
	stream.seekg(0, std::ios::beg);
	while (!stream.eof())
	{
		std::string str,hss;
		uint64_t hash;
		while (!stream.eof())
		{
			std::getline(stream, hss,',');
			if (hss.size() == 0)
				break;
			std::getline(stream, str);
			hash = std::stoull(hss.c_str(), nullptr, 16);
		}
		map->insert(std::pair<uint64_t, std::string>(hash, str));
	}
	return map;
}

std::map<uint64_t, std::string>* __stdcall fileToMap(std::string fileName)
{
	std::ifstream stream(fileName, std::ios::in);
	auto ret = fileToMap(stream);
	stream.close();
	return ret;
}

std::stringstream* __stdcall listToStream(std::list<std::string>* list)
{
	std::stringstream* stream = new std::stringstream(std::ios::in | std::ios::out);
	for (auto itr : *list)
		*stream << itr << std::endl;
	return stream;
}

std::stringstream* __stdcall mapToStream(std::map<uint64_t, std::string>* map)
{
	std::stringstream* stream = new std::stringstream(std::ios::in | std::ios::out);
	for (auto itr : *map)
		*stream << std::hex << itr.first << ',' << itr.second << std::endl;
	return stream;
}

errno_t __stdcall streamToFile(std::istream& stream, std::string fileName)
{
	if (!stream)return -1;
	std::ofstream file(fileName,std::ios::out|std::ios::binary);
	if (!file)return -1;
	file << stream.rdbuf();
	file.close();
	return 0;
}

errno_t __stdcall listToFile(std::list<std::string>* list, std::string fileName)
{
	return streamToFile(*listToStream(list), fileName);
}

errno_t __stdcall mapToFile(std::map<uint64_t, std::string>* map, std::string fileName)
{
	return streamToFile(*mapToStream(map), fileName);
}

size_t __stdcall getResolvedFolderCount(std::list<SCSSentries*>* entryList)
{
	size_t count = 0;
	for (auto itr : *entryList)
		if (itr->haveFileName()&&itr->getFileType()==scsFileType::folder)
			++count;
	return count;
}

size_t __stdcall getResolvedDirCount(std::list<SCSSentries*>* entryList)
{
	size_t count = 0;
	for (auto itr : *entryList)
		if (itr->haveFileName() && itr->getFileType() != scsFileType::folder)
			++count;
	return count;
}

size_t __stdcall getResolvedFileCount(std::list<SCSSentries*>* entryList)
{
	size_t count = 0;
	for (auto itr : *entryList)
		if (itr->haveFileName())
			++count;
	return count;
}

std::map<uint64_t,std::string>* __stdcall folderToMap(std::string root, std::string path)
{
	std::filesystem::path fileRoot(path);
	if (!std::filesystem::exists(fileRoot))
		return new std::map<uint64_t, std::string>();
	std::filesystem::directory_entry entry(path);
	if(entry.status().type()!=std::filesystem::file_type::directory)
		return new std::map<uint64_t, std::string>();
	std::filesystem::directory_iterator list(path);
	std::map<uint64_t, std::string>* map = new std::map<uint64_t, std::string>();
	for (auto itr : list)
	{
		std::string str = itr.path().string().substr(root.size() + 1);
		while (str.find_first_of('\\') != std::string::npos)
			str.replace(str.find_first_of("\\"), 1, "/");
		map->insert(std::pair<uint64_t, std::string>(getHash(str), str));
		if (itr.is_directory())
		{
			map->merge(*folderToMap(root, itr.path().string()));
		}
	}
	return map;
}

std::map<uint64_t, std::string>* __stdcall folderToMap(std::string folderName)
{
	return folderToMap(folderName, folderName);
}

std::map<uint64_t, std::string>* __stdcall listToMap(std::list<std::string> list)
{
	std::map<uint64_t, std::string>* map = new std::map<uint64_t, std::string>();
	for (auto itr : list)
	{
		map->insert(std::pair<uint64_t, std::string>(getHash(itr), itr));

		if (itr.size() > 3)
		{
			if (itr.substr(itr.size() - 3).compare("pmd") == 0)
				map->insert(std::pair<uint64_t, std::string>(getHash(itr.substr(0, itr.size() - 3) + "pmg"), itr.substr(0, itr.size() - 3) + "pmg"));
			if (itr.substr(itr.size() - 3).compare("dds") == 0)
			{
				map->insert(std::pair<uint64_t, std::string>(getHash(itr.substr(0, itr.size() - 3) + "tobj"), itr.substr(0, itr.size() - 3) + "tobj"));
				map->insert(std::pair<uint64_t, std::string>(getHash(itr.substr(0, itr.size() - 3) + "mat"), itr.substr(0, itr.size() - 3) + "mat"));
			}
			if (itr.substr(itr.size() - 3).compare("mat") == 0)
			{
				map->insert(std::pair<uint64_t, std::string>(getHash(itr.substr(0, itr.size() - 3) + "tobj"), itr.substr(0, itr.size() - 3) + "tobj"));
				map->insert(std::pair<uint64_t, std::string>(getHash(itr.substr(0, itr.size() - 3) + "dds"), itr.substr(0, itr.size() - 3) + "dds"));
			}
		}
		if (itr.size() > 4)
		{
			if (itr.substr(itr.size() - 4).compare("bank") == 0)
				map->insert(std::pair<uint64_t, std::string>(getHash(itr + ".guids"), itr + ".guids"));
			if (itr.substr(itr.size() - 4).compare("tobj") == 0)
			{
				map->insert(std::pair<uint64_t, std::string>(getHash(itr.substr(0, itr.size() - 4) + "dds"), itr.substr(0, itr.size() - 4) + "dds"));
				map->insert(std::pair<uint64_t, std::string>(getHash(itr.substr(0, itr.size() - 4) + "mat"), itr.substr(0, itr.size() - 4) + "mat"));
			}
		}
	}
	return map;
}

std::map<uint64_t, std::string>* __stdcall LogToMap(std::string fileName)
{
	return listToMap(*logToList(fileName, scsFileAccessMethod::inMemory));
}

errno_t __stdcall entryToFile(std::list<SCSSentries*>* entryList, std::string rootName, std::string sourceName)
{
	std::filesystem::path fileRoot(rootName);
	std::filesystem::create_directories(rootName);
	if (!std::filesystem::exists(rootName))throw("Invalid root");
	for (auto itr : *entryList)
	{
		std::string fileName;
		if (itr->haveFileName())
			fileName = itr->getFileName();
		else
			continue;
		while (fileName.find_first_of('/') != std::string::npos)
			fileName.replace(fileName.find_first_of("/"), 1, "\\");
		std::filesystem::path folder;
		if (itr->getFileType() == scsFileType::folder)
		{
			folder = rootName + "\\" + fileName;
			std::filesystem::create_directories(folder);
			continue;
		}
		if (fileName.find_first_of("\\") != std::string::npos)
		{
			folder = rootName + "\\" + fileName.substr(0, fileName.find_last_of("\\"));
			std::filesystem::create_directories(folder);
		}
		if (!itr->havecontent())
		{
			std::ifstream source(sourceName, std::ios::in | std::ios::binary);
			itr->getContent(source);
			source.close();
		}
		itr->inflateContent();
		if (!itr->havecontent())throw("No source File");
		if (itr->getFileType() == scsFileType::sii3nK || itr->getFileType() == scsFileType::siiEncrypted || itr->getFileType() == scsFileType::siiBinary)
		{
			itr->decryptSIIStream();
		}
		std::ofstream output(rootName+"\\"+fileName, std::ios::out|std::ios::binary);
		itr->getContent()->seekg(0, std::ios::end);
		size_t buffSize;
		char* buff = new char[buffSize=itr->getContent()->tellg()];
		itr->getContent()->seekg(0, std::ios::beg);
		itr->getContent()->read(buff, buffSize);
		output.write(buff, buffSize);
		delete[] buff;
		output.close();
		itr->dropContent();
	}
	return 0;
}

std::list<SCSSentries*>* __stdcall zipToEntries(std::istream& stream, EntryMode accessMode)
{
	std::list<SCSSentries*>* list = new std::list<SCSSentries*>();
	if (!stream)return list;
	stream.seekg(-0x16L, std::ios::end);
	uint32_t sign=0;
	while (stream.tellg())
	{
		stream.read((char*)&sign, 4);
		if (sign == 0x06054b50UL)
			break;
		stream.seekg(-5, std::ios::cur);
	}
	if (sign != 0x06054b50UL)return list;
	uint16_t fileCount;
	stream.seekg(4, std::ios::cur);
	stream.read((char*)&fileCount, 2);
	uint32_t startPos;
	stream.seekg(6, std::ios::cur);
	stream.read((char*)&startPos, 4);
	stream.seekg(startPos, std::ios::beg);
	for (uint16_t c = 0; c < fileCount; c++)
	{
		std::streampos entryPos = stream.tellg();
		std::streampos tempPos;
		uint16_t compression,crc, nameLen, fieldLen, commLen, offset,comSize,uncomSize;
		stream.seekg(6, std::ios::cur);
		stream.read((char*)&compression, 2);
		stream.seekg(4, std::ios::cur);
		stream.read((char*)&crc, 4);
		stream.read((char*)&comSize, 4);
		stream.read((char*)&uncomSize, 4);
		stream.read((char*)&nameLen, 2);
		stream.read((char*)&fieldLen, 2);
		stream.read((char*)&commLen, 2);
		stream.seekg(8, std::ios::cur);
		stream.read((char*)&offset, 4);
		char* name = new char[(size_t)nameLen + 1]{ 0 };
		stream.read(name, nameLen);
		scsSaveType saveType = scsSaveType::UncomFile;
		if (*(name + nameLen) == '/')
			saveType = scsSaveType::UncomFolder;
		if (compression)
			saveType = (scsSaveType)((uint8_t)saveType | 0x2);
		stream.seekg((size_t)fieldLen + (size_t)commLen, std::ios::cur);
		SCSSentries* entry = new SCSSentries(entryPos, getHash(name, nameLen), offset, saveType, crc, comSize, uncomSize, sourceType::ZIP);
		if ((accessMode & loadToMemory)==0x1)
		{
			tempPos = stream.tellg();
			char* content = new char[comSize];
			stream.seekg((size_t)offset+0x1EUL, std::ios::beg);
			stream.read(content, comSize);
			std::stringstream* contentStream = new std::stringstream(std::ios::in | std::ios::out | std::ios::binary);
			contentStream->write(content, comSize);
			delete[] content;
			stream.seekg(tempPos, std::ios::beg);
			entry->setContent(contentStream);
		}
		if ((accessMode & inflateStream) == 0x2)
			if (saveType == scsSaveType::ComFile)
			{
				if (!entry->havecontent())throw("contents not ready");
				entry->inflateContent();
			}
		if ((accessMode & identFile) == 0x4)
		{
			if (!entry->havecontent() || entry->isCompressed())throw("Contents is not ready");
			entry->identFileType();
		}
		scsFileType tempType = entry->getFileType();
		if ((accessMode & decryptSII) == 0x8)
		{
			if (!entry->havecontent() || entry->isCompressed())throw("Contents is not ready");
			if (entry->getFileType() == scsFileType::siiEncrypted || entry->getFileType() == scsFileType::siiBinary || entry->getFileType() == scsFileType::sii3nK)
				entry->decryptSIIStream();
		}
		if ((accessMode & getPathList) == 0x10)
		{
			if (!entry->havecontent() || entry->isCompressed())throw("Contents is not ready");
			entry->generatePathList();
		}
		if ((accessMode & dropContentAfterDone) == 0x20)
		{
			entry->dropContent();
			entry->setFileType(tempType);
		}
		list->push_back(entry);
	}
	return list;
}

std::list<SCSSentries*>* __stdcall zipToEntries(std::string fileName, EntryMode accessMode)
{
	std::ifstream stream(fileName, std::ios::in | std::ios::binary);
	auto returnValue = zipToEntries(stream, accessMode);
	stream.close();
	return returnValue;
}

std::list<SCSSentries*>* __stdcall fileToEntries(std::string rootName, EntryMode accessMode)
{
	std::list<SCSSentries*>* list = new std::list<SCSSentries*>();
	std::filesystem::path fileRoot(rootName);
	if (!std::filesystem::exists(rootName))throw("Invalid root");
	for (auto const& dir_entry : std::filesystem::recursive_directory_iterator(fileRoot))
	{
		std::string filename = dir_entry.path().string().substr(rootName.size()+1);
		scsSaveType saveType;
		if (dir_entry.is_directory())
			saveType = scsSaveType::UncomFolder;
		else
			saveType = scsSaveType::UncomFile;
		uint32_t uncompressedSize = (uint32_t)dir_entry.file_size();
		SCSSentries* entry = new SCSSentries(0, getHash(filename), 0, saveType, 0, uncompressedSize, uncompressedSize, sourceType::FILE);
		std::ifstream file(dir_entry, std::ios::in | std::ios::binary);
		if (accessMode & loadToMemory)
		{
			entry->getContent(file);
			file.close();
		}
		if ((accessMode & identFile))
		{
			if (!entry->havecontent())throw("Contents is not ready");
			entry->identFileType();
		}
		scsFileType tempType = entry->getFileType();
		if ((accessMode & decryptSII))
		{
			if (!entry->havecontent())throw("Contents is not ready");
			entry->decryptSIIStream();
		}
		if ((accessMode & getPathList))
		{
			if (!entry->havecontent())throw("Contents is not ready");
			entry->generatePathList();
		}
		if ((accessMode & dropContentAfterDone))
		{
			entry->dropContent();
			entry->setFileType(tempType);
		}
		list->push_back(entry);

	}
	return list;
}

std::list<std::string>* __stdcall zipToList(std::istream& stream)
{
	auto list = new std::list<std::string>();
	if (!stream)return list;
	stream.seekg(-0x16L, std::ios::end);
	uint32_t sign = 0;
	while (stream.tellg())
	{
		stream.read((char*)&sign, 4);
		if (sign == 0x06054b50UL)
			break;
		stream.seekg(-5, std::ios::cur);
	}
	if (sign != 0x06054b50UL)return list;
	uint16_t fileCount;
	stream.seekg(4, std::ios::cur);
	stream.read((char*)&fileCount, 2);
	uint32_t startPos;
	stream.seekg(6, std::ios::cur);
	stream.read((char*)&startPos, 4);
	stream.seekg(startPos, std::ios::beg);
	for (uint16_t c = 0; c < fileCount; c++)
	{
		std::streampos entryPos = stream.tellg();
		std::streampos tempPos;
		uint16_t compression, crc, nameLen, fieldLen, commLen, offset, comSize, uncomSize;
		stream.seekg(6, std::ios::cur);
		stream.read((char*)&compression, 2);
		stream.seekg(4, std::ios::cur);
		stream.read((char*)&crc, 4);
		stream.read((char*)&comSize, 4);
		stream.read((char*)&uncomSize, 4);
		stream.read((char*)&nameLen, 2);
		stream.read((char*)&fieldLen, 2);
		stream.read((char*)&commLen, 2);
		stream.seekg(8, std::ios::cur);
		stream.read((char*)&offset, 4);
		char* name = new char[(size_t)nameLen + 1]{ 0 };
		stream.read(name, nameLen);
		pushToList(list,std::string(name,nameLen));
		stream.seekg((size_t)fieldLen+ (size_t)commLen, std::ios::cur);
	}
	return list;
}

std::list<std::string>* __stdcall zipToList(std::string fileName)
{
	std::ifstream stream(fileName, std::ios::in | std::ios::binary);
	auto returnValue = zipToList(stream);
	stream.close();
	return returnValue;
}

std::list<std::string>* __stdcall mapToList(std::map<uint64_t, std::string>* map)
{
	auto list = new std::list<std::string>();
	for (auto itr : *map)
		list->push_back(itr.second);
	list->sort();
	list->unique();
	return list;
}

std::map<uint64_t, std::string>* __stdcall zipToMap(std::istream& stream)
{
	auto map = new std::map<uint64_t, std::string>();
	if (!stream)return map;
	for (auto itr : *zipToList(stream))
		map->insert(std::pair(getHash(itr), itr));
	return map;
}

std::map<uint64_t, std::string>* __stdcall zipToMap(std::string fileName)
{
	std::ifstream stream(fileName, std::ios::in | std::ios::binary);
	auto returnValue = zipToMap(stream);
	stream.close();
	return returnValue;
}

/*errno_t __stdcall entryToScss(std::list<SCSSentries*>* entryList, std::string fileName)
{
	std::ofstream stream(fileName, std::ios::out | std::ios::binary);
	char init[] = {0x53, 0x43, 0x53, 0x23, 0x01, 0x00, 0x00, 0x00, 0x43, 0x49, 0x54, 0x59};
	char zero[0xFF0] = { 0x00 };
	uint32_t init_count = entryList->size();
	stream.write(init, 0xC);
	stream.write((char*)&init_count, 0x4);
	stream.write(zero, 0xFF0);
	uint64_t offset = 0;
	for (auto itr : *entryList)
	{
		uint64_t hashcode = itr->getHashCode();
		stream.write((char*)&hashcode, 0x08);
		stream.write((char*)&offset, 0x08);
		uint32_t save_type = 0;
		save_type += (itr->isCompressed() * 2);
		save_type += (itr->getFileType() != scsFileType::folder);
		stream.write((char*)&save_type, 0x4);
		uint32_t crc = itr->getCrc();
		stream.write((char*)&crc, 0x4);
		uint32_t size = itr->getUncompressedSize();
		uint32_t zsize = itr->getCompressedSize();
		stream.write((char*)&size, 0x4);
		if (itr->isCompressed())
		{
			stream.write((char*)&zsize, 0x4);
			offset += zsize;
		}
		else
		{
			stream.write((char*)&size, 0x4);
			offset += size;
		}
	}
	for (auto itr : *entryList)
	{
		if(itr->isCompressed())
			stream.write(itr->getContent(),itr->getCompressedSize())
	}
}*/