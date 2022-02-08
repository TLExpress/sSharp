#include "scsFileAccess.hpp"


namespace scsFileAccess
{

	SCSEntry::SCSEntry(bool have_content, bool have_file_name, bool have_path_list, bool pass, size_t entry_pos, uint64_t hashcode, size_t file_pos, uint32_t compressed_size, uint32_t uncompressed_size, CompressedType compressed_type,
		bool decrypted, scsFileAccess::SaveType save_type, uint32_t crc, scsFileAccess::SourceType source_type, string sourcePath, scsFileAnalyzer::FileType file_type, SCSContent content, string file_name, SCSPathList path_list)
	{
		_have_content = have_content;
		_have_file_name = have_file_name;
		_have_path_list = have_path_list;

		if (have_content)
			_content = content;
		if (have_file_name)
		{
			_file_name = file_name;
			_hashcode = getHash(_file_name);
		}
		else
			_hashcode = hashcode;
		if (have_path_list)
			_path_list = path_list;
		_compressed_type = compressed_type;
		_entry_pos = entry_pos;
		_file_pos = file_pos;
		_output_size = _compressed_size = compressed_size;
		_uncompressed_size = uncompressed_size;
		_decrypted = decrypted;
		_save_type = save_type;
		_crc = crc;
		_source_type = source_type;
		if (source_type != SourceType::NoSource)
			_source_path = sourcePath;
		_file_type = file_type;
	}

	SCSEntry::SCSEntry(string source_path, size_t entry_pos, uint16_t access_mode, scsFileAccess::SourceType source)
	{
		try
		{
			_source_path = source_path;
			_entry_pos = entry_pos;
			_source_type = source;
			if (source == SourceType::SCS)
			{
				std::ifstream stream(source_path, ios::in | ios::binary);
				stream.seekg(entry_pos, ios::beg);
				stream.read((char*)&(_hashcode), sizeof(uint64_t));
				stream.read((char*)&(_file_pos), sizeof(size_t));
				if (sizeof(size_t) < 8) stream.seekg(8 - sizeof(size_t), ios::cur);
				uint32_t tempStype;
				stream.read((char*)&tempStype, sizeof(uint32_t));
				_save_type = (scsFileAccess::SaveType)(tempStype & 0x3);
				if (((uint8_t)_save_type & 2) == 2)
					_compressed_type = CompressedType::SCS;
				stream.read((char*)&(_crc), sizeof(uint32_t));
				stream.read((char*)&(_uncompressed_size), sizeof(uint32_t));
				stream.read((char*)&(_compressed_size), sizeof(uint32_t));
				stream.close();
			}
			else if (source == SourceType::ZIP)
			{
				std::ifstream stream(source_path, ios::in | ios::binary);
				stream.seekg(entry_pos + 0xA, ios::beg);
				uint16_t com;
				stream.read((char*)&com, 0x2);
				if (com == 8)
					_compressed_type = CompressedType::ZIP;
				stream.seekg(0x8, ios::cur);
				stream.read((char*)&_compressed_size, 0x4);
				stream.read((char*)&_uncompressed_size, 0x4);
				uint16_t namelen, fieldlen;
				stream.read((char*)&namelen, 0x2);
				stream.read((char*)&fieldlen, 0x2);
				stream.seekg(0xE, ios::cur);
				Buff tname(new char[(size_t)namelen + 1]{ 0 });
				stream.read(tname.get(), namelen);
				_file_name.assign(tname.get(), namelen);
				if (_file_name.at(namelen - 1) == '/')
				{
					_file_name = _file_name.substr(0, namelen - 1);
					if (_compressed_type == CompressedType::ZIP)
						_save_type = SaveType::ComFolder;
					else
						_save_type = SaveType::UncomFolder;
				}
				else
				{
					if (_compressed_type == CompressedType::ZIP)
						_save_type = SaveType::ComFile;
					else
						_save_type = SaveType::UncomFile;
				}
				_have_file_name = true;
				_hashcode = getHash(_file_name);
				stream.seekg(-(namelen + 0x4), ios::cur);
				stream.read((char*)&_file_pos, 0x4);
				stream.seekg(_file_pos + 0x1A, ios::beg);
				stream.read((char*)&namelen, 0x2);
				stream.read((char*)&fieldlen, 0x2);
				_file_pos += 0x1EULL + namelen + fieldlen;
				stream.close();
			}
			_output_size = _compressed_size;
			_content = nullptr;
			if ((access_mode & EntryMode::foldersOnly) && _save_type != SaveType::UncomFolder && _save_type != SaveType::ComFolder)
			{
				_pass = true;
				return;
			}
			namespace em = EntryMode;
			receiveContent();
			if (access_mode & em::inflateStream)
				if (((uint8_t)_save_type & 2) == 2)
					inflateContent();
			if (access_mode & em::identFile)
				identFileType();
			if (access_mode & em::identByFileName)
				identByFileName();
			auto tempType = _file_type;
			if (access_mode & em::decryptSII)
				decryptSII();
			if (access_mode & em::getPathList)
				generatePathList();
			if (access_mode & em::calculateCrc)
				calcCrc();
			if (!(access_mode & em::loadToMemory))
			{
				dropContent();
				_file_type = tempType;
			}
		}
		catch (std::istream::failure e)
		{
			std::cerr << "Load error" << std::endl;
			//還沒想到要寫什麼
		}
	}

	SCSEntry::SCSEntry(string filePath, string rootPath, uint16_t access_mode)
	{
		_source_path = filePath;
		_entry_pos = _file_pos = 0;
		_source_type = SourceType::File;
		_file_name.assign(filePath.substr(rootPath.size() + 1));
		_have_file_name = true;
		while (_file_name.find_first_of('\\') != string::npos)
			_file_name.replace(_file_name.find_first_of("\\"), 1, "/");
		_hashcode = getHash(_file_name);
		std::filesystem::directory_entry entry(filePath);
		if (entry.is_directory())
		{
			_save_type = SaveType::UncomFolder;
			_output_size = _uncompressed_size = _compressed_size = 0;
		}
		else
		{
			_save_type = SaveType::UncomFile;
			_output_size = _uncompressed_size = _compressed_size = (uint32_t)entry.file_size(); //加入大小檢查
		}
		_content = nullptr;
		if ((access_mode & EntryMode::foldersOnly) && _save_type != SaveType::UncomFolder && _save_type !=SaveType::ComFolder)
		{
			_pass = true;
			return;
		}
		if (_save_type == SaveType::UncomFile)
			receiveContent();
		namespace em = EntryMode;
		if (access_mode & em::identFile)
			identFileType();
		if (access_mode & em::identByFileName)
			identByFileName();
		auto tempType = _file_type;
		if (access_mode & em::decryptSII)
			decryptSII();
		if (access_mode & em::getPathList)
			generatePathList();
		if (access_mode & em::calculateCrc)
			calcCrc();
		if (!(access_mode & em::loadToMemory))
		{
			dropContent();
			_file_type = tempType;
		}
	}

	SCSEntry::~SCSEntry()
	{
		_content = nullptr;
		_path_list = nullptr;
	}

	void __stdcall SCSEntry::generatePathList()
	{
		if (_pass)
			return;
		if (_have_path_list)return;
		auto p = _compressed_type != CompressedType::Uncompressed ? inflatedContent() : (_have_content ? _content : receivedContent());
		if (_file_type == FileType::folder)
			_path_list = scsFileAnalyzer::extractFolderToList(*p);
		else
			_path_list = sfan::extractToList(*p);
		_have_path_list = true;
		return;
	}

	void __stdcall SCSEntry::identFileType()
	{
		if (_save_type == SaveType::ComFolder || _save_type == SaveType::UncomFolder)
		{
			_file_type = FileType::folder;
			return;
		}
		auto p = _have_content ? _content : receivedContent();
		_file_type = sfan::scsAnalyzeStream(*p);
		return;
	}

	void __stdcall SCSEntry::identByFileName()
	{
		if (!_have_file_name)
			return;
		string exname = _file_name.substr(_file_name.find_last_of('.') + 1);
		if (exname == "sii" || exname == "sui")
			identFileType();
		else if (exname == "pmd")
			_file_type = FileType::pmd;
		else if (exname == "mat")
			_file_type = FileType::mat;
		else if (exname == "tobj")
			_file_type = FileType::tobj;
		else if (exname == "jpg")
			_file_type = FileType::jpg;
		else if (exname == "pmg")
			_file_type = FileType::pmg;
		else if (exname == "dds")
			_file_type = FileType::dds;
		return;
	}

	SCSContent __stdcall SCSEntry::inflateContent()
	{
		auto temp = _content;
		_content = inflatedContent();
		_compressed_type = CompressedType::Uncompressed;
		_output_size = _uncompressed_size;
		_have_content = true;
		return _content;
	}

	SCSContent __stdcall SCSEntry::inflatedContent()
	{
		auto p = receivedContent();
		if (_compressed_type == CompressedType::Uncompressed)
			return p;
		p->seekg(0, ios::end);
		uint32_t size = (uint32_t)p->tellg();
		Buff buff(new char[size]);
		p->seekg(0, ios::beg);
		p->read(buff.get(), size);
		p->seekg(0, ios::beg);
		size_t bsize = compressBound(_uncompressed_size);
		Buff ubuff(new char[bsize]);
		uncompress3((Bytef*)ubuff.get(), (uLongf*)&bsize, (Bytef*)buff.get(), (uLong)size, _compressed_type == CompressedType::SCS ? 15 : -15);

		p = make_shared<stringstream>(ios::in | ios::out | ios::binary);
		p->seekp(0, ios::beg);
		p->write(ubuff.get(), bsize);
		return p;
	}

	SCSContent __stdcall SCSEntry::deflateContent(scsFileAccess::CompressedType compressed_type)
	{
		auto temp = _content;
		_content = deflatedContent(compressed_type);
		_compressed_type = compressed_type;
		_content->seekg(0, ios::end);
		_output_size = (uint32_t)_content->tellg();
		_have_content = true;
		return _content;
	}

	SCSContent __stdcall SCSEntry::deflatedContent(scsFileAccess::CompressedType compressed_type)
	{
		auto p = receivedContent();
		if (_compressed_type != CompressedType::Uncompressed)
			return p;
		p->seekg(0, ios::end);
		uint32_t size = (uint32_t)p->tellg();
		p->seekg(0, ios::beg);
		Buff buff(new char[_uncompressed_size]);
		p->read(buff.get(), _uncompressed_size);
		size_t bsize = compressBound(_uncompressed_size);
		Buff ubuff(new char[bsize]);
		compress3((Bytef*)ubuff.get(), (uLongf*)&bsize, (Bytef*)buff.get(), (uLong)size, compressed_type == CompressedType::SCS ? 15 : -15);
		p = make_shared<stringstream>(ios::in | ios::out | ios::binary);
		p->seekp(0, ios::beg);
		p->write(ubuff.get(), bsize);
		return p;
	}

	SCSContent __stdcall SCSEntry::decryptSII()
	{
		auto temp_type = _file_type;
		if (_file_type != FileType::sii3nK)
			return _content;
		auto temp = _content;
		_content = decryptedSII();
		_have_content = true;
		_compressed_type = CompressedType::Uncompressed;
		identFileType();
		if (temp_type != _file_type)
		{
			_decrypted = true;
			_output_size = _uncompressed_size - 6;
		}
		return _content;
	}

	SCSContent __stdcall SCSEntry::decryptedSII()
	{

		auto p = _compressed_type != CompressedType::Uncompressed ? inflatedContent() : receivedContent();

#ifdef LOAD_SIIDECRYPT

		size_t decryptSize = 0x1000000;
		Buff in(new char[uncompressedSize]);
		Buff out(new char[decryptSize]);
		p->seekg(0, ios::beg);
		p->read(in.get(), uncompressedSize);
		uint32_t ret = DecryptAndDecodeMemory(in, uncompressedSize, out, &decryptSize);
		p = make_shared<stringstream>(ios::in | ios::out | ios::binary);
		p->write(out, decryptSize);
		return p;

#else

		if (_file_type != FileType::sii3nK)
			return p;
		S3nKTranscoder transcoder;
		if (transcoder.is3nKStream(*p))
		{
			SCSContent decrypted_stream = make_shared<stringstream>(ios::in | ios::out | ios::binary);
			transcoder.decodeStream(*p, *decrypted_stream, false);
			auto temp = p;
			p = decrypted_stream;
			if (sfan::scsAnalyzeStream(*p) == FileType::sii)
				return p;
			return temp;
		}
		return p;

#endif

	}

	SCSContent __stdcall SCSEntry::encryptSII()
	{
		auto temp_type = _file_type;
		if (_file_type != FileType::sii && _file_type != FileType::sui)
			return _content;
		auto temp = _content;
		_content = encryptedSII();
		_file_type = FileType::sii3nK;
		if (temp_type != _file_type)
		{
			if(_decrypted == false)
				_output_size = _uncompressed_size + 6;
			else
			{
				_output_size = _uncompressed_size;
				_decrypted = false;
			}
		}
		return _content;
	}

	SCSContent __stdcall SCSEntry::encryptedSII()
	{

		auto p = _compressed_type != CompressedType::Uncompressed ? inflatedContent() : receivedContent();

		if (_file_type != FileType::sii && _file_type != FileType::sui)
			return p;
		S3nKTranscoder transcoder;
		if (!transcoder.is3nKStream(*p))
		{
			SCSContent encrypted_stream = make_shared<stringstream>(ios::in | ios::out | ios::binary);
			transcoder.encodeStream(*p, *encrypted_stream, false);
			auto temp = p;
			p = encrypted_stream;
			if (sfan::scsAnalyzeStream(*p) == FileType::sii3nK)
				return p;
			return temp;
		}
		return p;
	}

	SCSContent __stdcall SCSEntry::receiveContent()
	{
		auto temp = _content;
		_content = receivedContent();
		_have_content = true;
		return _content;
	}

	SCSContent __stdcall SCSEntry::receivedContent()
	{
		if (_have_content)
			return _content;
		ifstream stream(_source_path, ios::in | ios::binary);
		SCSContent p = make_shared<stringstream>(ios::in | ios::out | ios::binary);
		Buff buff(new char[_compressed_size]);
		stream.seekg(_file_pos, ios::beg);
		stream.read(buff.get(), _compressed_size);
		stream.close();
		p->write(buff.get(), _compressed_size);
		return p;
	}

	bool __stdcall SCSEntry::searchName(SCSDictionary map)
	{
		auto itr = map->find(_hashcode);
		if (itr != map->end())
		{
			_file_name = itr->second;
			_have_file_name = true;
			return true;
		}
		return false;
	}

	uint32_t __stdcall SCSEntry::toAbsPath()
	{
		uint32_t counter=0;
		if (!_have_file_name)return -1;
		for (auto& itr : *_path_list)
		{
			if (itr.at(0) == '~' || itr.at(0) == '*')
			{
				if (_file_type == FileType::folder)
					itr = _file_name + (_file_name.size() == 0 ? "" : "/") + itr.substr(1);
				else
					itr = (_file_name.find('/') == std::string::npos) ? itr.substr(1) : (_file_name.substr(0, _file_name.find_last_of('/') + 1) + itr.substr(1));
				counter++;
			}
		}
		sort(_path_list->begin(), _path_list->end());
		_path_list->erase(remove_if(_path_list->begin(), _path_list->end(),[](std::string s) {return s.at(0) == '~' || s.at(0) == '*'; }), _path_list->end());
		return counter;
	}

	bool __stdcall SCSEntry::pathListAllAbs()
	{
		if (_path_list == nullptr)return true;
		for (auto itr : *_path_list)
			if (itr.at(0) == '~' || itr.at(0) == '*')
				return false;
		return true;
	}

	uint32_t __stdcall SCSEntry::calcCrc()
	{
		uint32_t size = decrypted?_uncompressed_size-6: _uncompressed_size;
		auto p = inflatedContent();
		Buff buff(new char[size]);
		p->seekg(0, ios::beg);
		p->read(buff.get(), size);
		_crc = crc32(0, (const Bytef*)buff.get(), size);
		return _crc;
	}

	void __stdcall SCSEntry::dropContent()
	{
		if (_source_type != SourceType::NoSource)
			dropContentForce();
		return;
	}

	void __stdcall SCSEntry::dropContentForce()
	{
		_content = nullptr;
		_have_content = false;
		if (_decrypted)
			_file_type = FileType::sii3nK;
		_decrypted = false;
		_output_size = _compressed_size;
		if ((_save_type == SaveType::ComFile || _save_type == SaveType::ComFolder) && _source_type == SourceType::SCS)
			_compressed_type = CompressedType::SCS;
		else if (_save_type == SaveType::ComFile && _source_type == SourceType::ZIP)
			_compressed_type = CompressedType::ZIP;
		else
			_compressed_type = CompressedType::Uncompressed;
		return;
	}
}