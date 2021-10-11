#include "scsFileAccess.hpp"

SCSSentries::SCSSentries(size_t entryPos, uint64_t hashcode, size_t filePos, scsSaveType saveType, uint32_t crc, uint32_t compressedSize, uint32_t uncompressedSize)
{
	this->entryPos = entryPos;
	this->hashcode = hashcode;
	this->saveType = saveType;
	this->crc = crc;
	this->compressedSize = compressedSize;
	this->uncompressedSize = uncompressedSize;
	this->content = nullptr;
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
	stream.read((char*)&(this->compressedSize), sizeof(uint32_t));
	this->content = nullptr;
	if ((accessMode & loadToMemory) == 0x1)
	{
		this->content = new std::stringstream(std::ios::in | std::ios::out | std::ios::binary);
		stream.seekg(filePos, std::ios::beg);
		char* buff = new char[compressedSize];
		stream.read(buff, compressedSize);
		this->content->write(buff, compressedSize);
		delete[] buff;
		_haveContent = true;
	}
	if ((accessMode & inflateStream) == 0x2)
	{
		if (((uint8_t)saveType & 2) == 2)
		{
			char* buff = new char[compressedSize];
			if (!havecontent())
			{
				stream.seekg(filePos, std::ios::beg);
				stream.read(buff, compressedSize);
			}
			else
			{
				this->content->seekg(0, std::ios::beg);
				this->content->read(buff, compressedSize);
			}
			size_t bsize = compressBound(uncompressedSize);
			char* ubuff = new char[bsize];
			uncompress((Bytef*)ubuff, (uLongf*)&bsize, (Bytef*)buff, (uLong)compressedSize);
			delete[] buff;
			this->content->seekg(0, std::ios::beg);
			this->content->clear();
			delete this->content;
			this->content = new std::stringstream(std::ios::in | std::ios::out | std::ios::binary);
			this->content->seekg(0, std::ios::beg);
			this->content->write(ubuff, bsize);
			delete[] ubuff;
			_contentCompressed = false;
		}
	}
	if ((accessMode & identFile) == 0x4)
	{
		if (saveType == scsSaveType::ComFolder || saveType == scsSaveType::UncomFolder)
		{
			fileType = scsFileType::folder;
		}
		else if (!havecontent())
		{
			stream.seekg(filePos, std::ios::beg);
			char* buff = new char[compressedSize];
			stream.read(buff, compressedSize);
			this->fileType = scsAnalyzeBuff(buff, compressedSize);
			delete[] buff;
		}
		else
		{
			this->content->seekg(0, std::ios::beg);
			this->fileType = scsAnalyzeStream(*this->content);
			this->content->seekg(0, std::ios::beg);
		}
	}
	if ((accessMode & decryptSII) == 0x8)
	{
		if (!havecontent()|| _contentCompressed)throw("Contents is not ready");
		if (this->fileType == scsFileType::siiEncrypted|| this->fileType == scsFileType::siiBinary|| this->fileType == scsFileType::sii3nK)
		{
			this->content->seekg(0, std::ios::beg);
			std::fstream ts("temp", std::ios::out|std::ios::binary);
			ts << this->content->rdbuf();
			ts.close();
			DecryptAndDecodeFileInMemory((char*)"temp", (char*)"temp");
			ts.open("temp", std::ios::in | std::ios::binary);
			this->content->clear();
			delete this->content;
			this->content = new std::stringstream(std::ios::in | std::ios::out | std::ios::binary);
			this->content->seekg(0, std::ios::beg);
			*content << ts.rdbuf();
			ts.close();
			remove("temp");
			this->content->seekg(0, std::ios::end);
			uncompressedSize = (uint32_t)this->content->tellg();
			this->content->seekg(0, std::ios::beg);
			this->fileType = scsFileType::sii;
		}
	}
	if ((accessMode & getPathList) == 0x10)
	{
		if (!havecontent() || _contentCompressed)throw("Contents is not ready");
		this->generatePathList();
	}
	if ((accessMode & dropContentAfterDone) == 0x20)
		this->dropContent();
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

errno_t __stdcall SCSSentries::identFileType(std::istream& stream, std::streampos pos)
{
	if (saveType == scsSaveType::ComFolder || saveType == scsSaveType::UncomFolder)
	{
		fileType = scsFileType::folder;
		return 0;
	}
	stream.seekg(pos, std::ios::beg);
	char* buff = new char[compressedSize];
	stream.read(buff, compressedSize);
	fileType = scsAnalyzeBuff(buff, compressedSize);
	delete[] buff;
	return 0;
}

errno_t __stdcall SCSSentries::inflateContent()
{
	if (!havecontent())return -1;
	if (_contentCompressed == true)
		inflateContent(*content, 0);
	return 0;
}

errno_t __stdcall SCSSentries::inflateContent(std::istream& stream, std::streampos pos)
{
	if (!stream)return -1;
	if (_contentCompressed == false)return 0;
	char* buff = new char[compressedSize];
		stream.seekg(pos, std::ios::beg);
		stream.read(buff, compressedSize);
	size_t bsize = compressBound(uncompressedSize);
	char* ubuff = new char[bsize];
	uncompress((Bytef*)ubuff, (uLongf*)&bsize, (Bytef*)buff, (uLong)compressedSize);
	delete[] buff;
	content->seekg(0, std::ios::beg);
	content->clear();
	delete[] content;
	content = new std::stringstream(std::ios::in | std::ios::out | std::ios::binary);
	content->seekg(0, std::ios::beg);
	content->write(ubuff, bsize);
	delete[] ubuff;
	_contentCompressed = false;
	return 0;
}

errno_t __stdcall SCSSentries::getContent(std::istream& stream)
{
	return getContent(stream, filePos);
}

errno_t __stdcall SCSSentries::getContent(std::istream& stream, std::streampos pos)
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
	return 0;
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
				itr = (fileName->find('/') == std::string::npos) ? itr.substr(1) : (fileName->substr(fileName->find_last_of('/') + 1) + itr.substr(1));
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

std::list<SCSSentries*>* __stdcall getEntryList(std::istream& stream, EntryMode accessMode)
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

std::list<SCSSentries*>* __stdcall getEntryListFromFile(std::string fileName, EntryMode accessMode)
{
	std::ifstream stream(fileName, std::ios::in | std::ios::binary);
	auto returnValue = getEntryList(stream, accessMode);
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

std::list<std::string>* __stdcall streamToList(std::istream& stream)
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
	auto ret = streamToList(stream);
	stream.close();
	return ret;
}

std::map<uint64_t, std::string>* __stdcall csvToMap(std::istream& stream)
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
	auto ret = csvToMap(stream);
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

std::stringstream* __stdcall mapToCsv(std::map<uint64_t, std::string>* map)
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
	return streamToFile(*mapToCsv(map), fileName);
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

std::map<uint64_t,std::string>* __stdcall getFolders(std::string root, std::string path)
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
			map->merge(*getFolders(root, itr.path().string()));
		}
	}
	return map;
}

std::map<uint64_t, std::string>* __stdcall folderToMap(std::string folderName)
{
	return getFolders(folderName, folderName);
}

std::map<uint64_t, std::string>* __stdcall listToMap(std::list<std::string> list)
{
	std::map<uint64_t, std::string>* map = new std::map<uint64_t, std::string>();
	for (auto itr : list)
		map->insert(std::pair<uint64_t, std::string>(getHash(itr), itr));
	return map;
}

std::map<uint64_t, std::string>* __stdcall LogToMap(std::string fileName)
{
	return listToMap(*logToList(fileName, scsFileAccessMethod::inMemory));
}