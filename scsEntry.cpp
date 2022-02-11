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
				if (!stream)
					throw SCSSException(__func__, "source is unavailable");
				stream.seekg(entry_pos, ios::beg).read((char*)&(_hashcode), 8).read((char*)&(_file_pos), sizeof(size_t));
				if (sizeof(size_t) < 8) stream.seekg(8 - sizeof(size_t), ios::cur);
				uint32_t tempStype;
				stream.read((char*)&tempStype, 4);
				_save_type = (scsFileAccess::SaveType)(tempStype & 0x3);
				if (((uint8_t)_save_type & 2) == 2)
					_compressed_type = CompressedType::SCS;
				stream.read((char*)&(_crc), 4).read((char*)&(_uncompressed_size), 4).read((char*)&(_compressed_size), 4);
				stream.close();
			}
			else if (source == SourceType::ZIP)
			{
				std::ifstream stream(source_path, ios::in | ios::binary);
				if (!stream)
					throw SCSSException(__func__, "source is unavailable");

				uint16_t com;
				stream.seekg(entry_pos + 0xA, ios::beg).read((char*)&com, 0x2);
				if (com == 8)
					_compressed_type = CompressedType::ZIP;
				stream.seekg(0x8, ios::cur).read((char*)&_compressed_size, 0x4).read((char*)&_uncompressed_size, 0x4);

				uint16_t namelen, fieldlen;
				stream.read((char*)&namelen, 0x2).read((char*)&fieldlen, 0x2);

				Buff tname(new char[(size_t)namelen + 1]{ 0 });
				stream.seekg(0xE, ios::cur).read(tname.get(), namelen);
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

				stream.seekg(-(namelen + 0x4), ios::cur).read((char*)&_file_pos, 0x4);
				stream.seekg(_file_pos + 0x1A, ios::beg).read((char*)&namelen, 0x2).read((char*)&fieldlen, 0x2);
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
		catch (SCSSException) {}
		catch (...) { sfan::ueMessage(__func__); }
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
		stdfs::directory_entry entry(filePath);
		if (entry.is_directory())
		{
			_save_type = SaveType::UncomFolder;
			_output_size = _uncompressed_size = _compressed_size = 0;
		}
		else
		{
			_save_type = SaveType::UncomFile;
			_output_size = _uncompressed_size = _compressed_size = (uint32_t)entry.file_size(); 
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
	{}

	SCSPathList SCSEntry::generatePathList()
	{
		if (_pass)
			return nullptr;
		if (_have_path_list)return nullptr;
		auto p = inflatedContent();
		if (_file_type == FileType::folder)
			_path_list = scsFileAnalyzer::extractFolderToList(*p);
		else
			_path_list = sfan::extractToList(*p);
		_have_path_list = true;
		return _path_list;
	}

	FileType SCSEntry::identFileType()
	{
		if (_save_type == SaveType::ComFolder || _save_type == SaveType::UncomFolder)
			return _file_type = FileType::folder;

		return _file_type = sfan::scsAnalyzeStream(*receivedContent());
	}

	FileType SCSEntry::identByFileName()
	{
		if (!_have_file_name)
			return _file_type;
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
		return _file_type;
	}

	SCSContent SCSEntry::inflateContent()
	{
		auto temp = _content;
		_content = inflatedContent();
		_compressed_type = CompressedType::Uncompressed;
		_output_size = _uncompressed_size;
		_have_content = true;
		return _content;
	}

	SCSContent SCSEntry::inflatedContent()
	{
		auto stream = receivedContent();
		try
		{
			if (!*stream)
				throw SCSSException(__func__, "stream is unavailable");
			if (_compressed_type == CompressedType::Uncompressed)
				return stream;

			uint32_t size = sfan::getStreamSize(*stream);
			Buff buff(new char[size]);
			stream->read(buff.get(), size);
			stream->seekg(0, ios::beg);

			size_t bsize = compressBound(_uncompressed_size);
			Buff ubuff(new char[bsize]);
			uncompress3((Bytef*)ubuff.get(), (uLongf*)&bsize, (Bytef*)buff.get(), (uLong)size, _compressed_type == CompressedType::SCS ? 15 : -15);

			stream = make_shared<stringstream>(ios::in | ios::out | ios::binary);
			stream->seekp(0, ios::beg).write(ubuff.get(), bsize);
		}
		catch (SCSSException) {}
		catch (...) { sfan::ueMessage(__func__); }
		return stream;
	}

	SCSContent SCSEntry::deflateContent(scsFileAccess::CompressedType compressed_type)
	{
		auto temp = _content;
		_content = deflatedContent(compressed_type);
		_compressed_type = compressed_type;
		_output_size = sfan::getStreamSize(*_content);
		_have_content = true;
		return _content;
	}

	SCSContent SCSEntry::deflatedContent(scsFileAccess::CompressedType compressed_type)
	{
		auto stream = receivedContent();
		try
		{
			if (!*stream)
				throw SCSSException(__func__, "stream is unavailable");
			if (_compressed_type != CompressedType::Uncompressed)
				return stream;
			Buff buff(new char[_uncompressed_size]);
			stream->read(buff.get(), _uncompressed_size);
			size_t bsize = compressBound(_uncompressed_size);
			Buff ubuff(new char[bsize]);
			compress3((Bytef*)ubuff.get(), (uLongf*)&bsize, (Bytef*)buff.get(), (uLong)_uncompressed_size, compressed_type == CompressedType::SCS ? 15 : -15);

			stream = make_shared<stringstream>(ios::in | ios::out | ios::binary);
			stream->seekp(0, ios::beg).write(ubuff.get(), bsize);
		}
		catch (SCSSException) {}
		catch (...) { sfan::ueMessage(__func__); }
		return stream;
	}

	SCSContent SCSEntry::decryptSII()
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

	SCSContent SCSEntry::decryptedSII()
	{
		auto stream = inflatedContent();
		auto temp = stream;
		try
		{
			if (!stream)
				throw SCSSException(__func__, "stream is unavailable");

#ifdef LOAD_SIIDECRYPT

			size_t decryptSize = 0x1000000;
			Buff in(new char[uncompressedSize]);
			Buff out(new char[decryptSize]);
			stream->seekg(0, ios::beg);
			stream->read(in.get(), uncompressedSize);
			uint32_t ret = DecryptAndDecodeMemory(in, uncompressedSize, out, &decryptSize);
			stream = make_shared<stringstream>(ios::in | ios::out | ios::binary);
			stream->write(out, decryptSize);
			return stream;

#else

			if (_file_type != FileType::sii3nK)
				return stream;
			S3nKTranscoder transcoder;
			if (transcoder.is3nKStream(*stream))
			{
				SCSContent decrypted_stream = make_shared<stringstream>(ios::in | ios::out | ios::binary);
				transcoder.decodeStream(*stream, *decrypted_stream, false);
				stream = decrypted_stream;
				if (sfan::scsAnalyzeStream(*stream) == FileType::sii)
					return stream;
				throw SCSSException(__func__, "decrypt function failed");
			}
#endif

		}
		catch (SCSSException) {}
		catch (...) { sfan::ueMessage(__func__); }
		return temp;
	}

	SCSContent SCSEntry::encryptSII()
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

	SCSContent SCSEntry::encryptedSII()
	{
		auto stream = inflatedContent();
		auto temp = stream;
		try
		{

			if (_file_type != FileType::sii && _file_type != FileType::sui)
				return stream;
			S3nKTranscoder transcoder;
			if (!transcoder.is3nKStream(*stream))
			{
				SCSContent encrypted_stream = make_shared<stringstream>(ios::in | ios::out | ios::binary);
				transcoder.encodeStream(*stream, *encrypted_stream, false);
				stream = encrypted_stream;
				if (sfan::scsAnalyzeStream(*stream) == FileType::sii3nK)
					return stream;
				throw SCSSException(__func__, "encrypt function failed");
			}
		}
		catch (SCSSException) {}
		catch (...) { sfan::ueMessage(__func__); }
		return temp;
	}

	SCSContent SCSEntry::receiveContent()
	{
		auto temp = _content;
		_content = receivedContent();
		_have_content = true;
		return _content;
	}

	SCSContent SCSEntry::receivedContent()
	{
		if (_have_content)
			return _content;

		ifstream source(_source_path, ios::in | ios::binary);
		SCSContent stream = make_shared<stringstream>(ios::in | ios::out | ios::binary);

		try
		{
			if (!source || !stream)
				throw SCSSException(__func__, "source is unavailable");
			Buff buff(new char[_compressed_size]);
			source.seekg(_file_pos, ios::beg).read(buff.get(), _compressed_size);
			stream->write(buff.get(), _compressed_size);
		}
		catch (SCSSException) {}
		catch (...) { sfan::ueMessage(__func__); }
		return stream;
	}

	bool SCSEntry::searchName(SCSDictionary map)
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

	uint32_t SCSEntry::toAbsPath()
	{
		if (!_have_file_name || !_have_path_list)return 0;

		uint32_t counter = 0;
		for (auto& itr : *_path_list)
			if (itr.at(0) == '~' || itr.at(0) == '*')
			{
				if (_file_type == FileType::folder)
					itr = _file_name + (_file_name.size() == 0 ? "" : "/") + itr.substr(1);
				else
					itr = (_file_name.find('/') == std::string::npos) ? itr.substr(1) : (_file_name.substr(0, _file_name.find_last_of('/') + 1) + itr.substr(1));
				counter++;
			}
		return counter;
	}

	bool SCSEntry::pathListAllAbs()
	{
		if (_path_list == nullptr || _path_list->empty())return true;
		for (auto const& itr : *_path_list)
			if (itr.at(0) == '~' || itr.at(0) == '*')
				return false;
		return true;
	}

	uint32_t SCSEntry::calcCrc()
	{
		uint32_t size = decrypted?_uncompressed_size-6: _uncompressed_size;
		auto stream = inflatedContent();
		try
		{
			if (!stream)
				throw SCSSException(__func__, "stream is unavailable");
			Buff buff(new char[size]);
			stream->seekg(0, ios::beg).read(buff.get(), size);
			_crc = crc32(0, (const Bytef*)buff.get(), size);
		}
		catch (SCSSException) {}
		catch (...) { sfan::ueMessage(__func__); }
		return _crc;
	}

	void SCSEntry::dropContent()
	{
		if (_source_type != SourceType::NoSource)
			dropContentForce();
		return;
	}

	void SCSEntry::dropContentForce()
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