#include "scsFileAccess.hpp"

SCSSentries::SCSSentries(size_t entryPos, uint64_t hashcode, size_t filePos, scsSaveType saveType, uint32_t crc, uint32_t compressedSize, uint32_t uncompressedSize)
{
	this->entryPos = entryPos;
	this->hashcode = hashcode;
	this->saveType = saveType;
	this->crc = crc;
	this->compressedSize = compressedSize;
	this->uncompressedSize = uncompressedSize;
}

SCSSentries::SCSSentries(std::istream& stream, size_t entryPos, EntryMode accessmode)
{
	if (!stream)throw("StreamIOError");
	this->entryPos = entryPos;
	stream.seekg(entryPos, std::ios::beg);
	stream.read((char*)&(this->hashcode), sizeof(uint64_t));
	stream.read((char*)&(this->filePos), sizeof(uint64_t));
	uint32_t tempStype;
	stream.read((char*)&tempStype, sizeof(uint32_t));
	this->saveType = (scsSaveType)(tempStype&0x3);
	if ((uint8_t)saveType & 2 == 2)
		this->_contentCompressed = true;
	stream.read((char*)&(this->crc), sizeof(uint32_t));
	stream.read((char*)&(this->uncompressedSize), sizeof(uint32_t));
	stream.read((char*)&(this->compressedSize), sizeof(uint32_t));
	if (accessmode & loadToMemory == 1)
	{
		stream.seekg(filePos, std::ios::beg);
		this->content.setf(std::ios::in|std::ios::out|std::ios::binary);
		char* buff = new char[compressedSize];
		stream.read(buff, compressedSize);
		this->content.write(buff, compressedSize);
		delete[] buff;
		_haveContent = true;
	}
	if (accessmode & inflateStream == 2)
	{
		if ((uint8_t)saveType & 2 == 2)
		{
			char* buff = new char[compressedSize];
			if (!havecontent())
			{
				stream.seekg(filePos, std::ios::beg);
				stream.read(buff, compressedSize);
			}
			else
			{
				this->content.seekg(0, std::ios::beg);
				this->content.read(buff, compressedSize);
			}
			size_t bsize = compressBound(uncompressedSize);
			char* ubuff = new char[bsize];
			uncompress((Bytef*)ubuff, (uLongf*)&bsize, (Bytef*)buff, (uLong)compressedSize);
			delete[] buff;
			this->content.seekg(0, std::ios::beg);
			this->content.clear();
			this->content.seekg(0, std::ios::beg);
			this->content.write(ubuff, bsize);
			delete[] ubuff;
			_contentCompressed = false;
		}
	}
	if (accessmode & identFile == 4)
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
			this->content.seekg(0, std::ios::beg);
			this->fileType = scsAnalyzeStream(this->content);
			this->content.seekg(0, std::ios::beg);
		}
	}
	if (accessmode & decryptSII == 8)
	{
		if (!havecontent()|| _contentCompressed)throw("Contents is not ready");
		if (this->fileType == scsFileType::siiEncrypted|| this->fileType == scsFileType::siiBinary|| this->fileType == scsFileType::sii3nK)
		{
			char* buff = new char[uncompressedSize];
			char* obuff = nullptr;
			this->content.seekg(0, std::ios::beg);
			this->content.read(buff, this->uncompressedSize);
			this->content.seekg(0, std::ios::beg);
			size_t osize;
			DecryptAndDecodeMemory(buff, this->uncompressedSize, obuff, &osize);
			delete[] buff;
			this->uncompressedSize = osize;
			this->content.seekg(0, std::ios::beg);
			this->content.clear();
			this->content.seekg(0, std::ios::beg);
			this->content.write(obuff, this->uncompressedSize);
			delete[] obuff;
			this->fileType = scsFileType::sii;
		}
	}
	if (accessmode & getPathList == 16)
	{
		if (!havecontent() || _contentCompressed)throw("Contents is not ready");
		this->generatePathList();
	}
}

SCSSentries::~SCSSentries()
{
	delete pathList;
	content.clear();
}

errno_t SCSSentries::generatePathList()
{
	if (!havecontent() || _contentCompressed)return -1;
	pathList = extractToList(this->content);
	if (this->pathList != nullptr)
		this->_havePathList = true;
	return 0;
}

errno_t SCSSentries::identFileType()
{
	if (saveType == scsSaveType::ComFolder || saveType == scsSaveType::UncomFolder)
	{
		fileType = scsFileType::folder;
		return 0;
	}
	if (!havecontent() || _contentCompressed)return -1;
	content.seekg(0, std::ios::beg);
	fileType = scsAnalyzeStream(content);
	content.seekg(0, std::ios::beg);
	return 0;
}

errno_t SCSSentries::identFileType(std::istream& stream, std::streampos pos)
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

errno_t SCSSentries::inflateContent()
{
	if (!havecontent())return -1;
	if (_contentCompressed == true)
		inflateContent(content, 0);
	return 0;
}

errno_t SCSSentries::inflateContent(std::istream& stream, std::streampos pos)
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
	content.seekg(0, std::ios::beg);
	content.clear();
	content.seekg(0, std::ios::beg);
	content.write(ubuff, bsize);
	delete[] ubuff;
	_contentCompressed = false;
}

errno_t SCSSentries::getContent(std::istream& stream)
{
	return getContent(stream, filePos);
}

errno_t SCSSentries::getContent(std::istream& stream, std::streampos pos)
{
	stream.seekg(pos, std::ios::beg);
	content.setf(std::ios::in | std::ios::out | std::ios::binary);
	char* buff = new char[compressedSize];
	stream.read(buff, compressedSize);
	content.write(buff, compressedSize);
	delete[] buff;
	_haveContent = true;
}

errno_t SCSSentries::searchName(std::map<uint64_t, std::string>* map)
{
	if (map == nullptr)return -1;
	auto itr = map->find(hashcode);
	if (itr != map->end())
	{
		fileName = new std::string(itr->second);
		return 0;
	}
	return 1;
}

errno_t SCSSentries::toAbsPath()
{
	if (fileName == nullptr||pathList == nullptr)return -1;
	for (auto itr : *pathList)
	{
		if (itr.at(0) == '~' || itr.at(0) == '*')
		{
			if (fileType == scsFileType::folder)
				itr = *fileName + "/" + itr.substr(1);
			else
				itr = fileName->substr(0, fileName->find_last_of('/') + 1) + itr.substr(1);
		}
	}
	return 0;
}

bool SCSSentries::pathListAllAbs()
{
	if (pathList == nullptr)return true;
	for (auto itr : *pathList)
		if (itr.at(0) == '~' || itr.at(0) == '*')
			return false;
	return true;
}