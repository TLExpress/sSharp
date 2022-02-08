#include "scsFileAccess.hpp"

namespace scsFileAccess
{

	SCSEntryList __stdcall scssToEntries(string file_name, uint16_t access_mode)
	{
		try
		{
			ifstream stream(file_name, ios::in | ios::binary);
			SourceType source_type = SourceType::SCS;
			SCSEntryList entry_list = make_shared<_SCSEntryList>();
			uint32_t sign;
			uint32_t entryCount;

			stream.seekg(0, ios::beg);
			stream.read((char*)&sign, 4);
			if (sign != 0x23534353)throw("Invalid file");

			stream.seekg(0xC, ios::beg);
			stream.read((char*)&entryCount, sizeof(entryCount));

			stream.seekg(0x1000, ios::beg);
			std::streampos pos = stream.tellg();
			stream.close();
			for (uint32_t counter = 0; counter < entryCount; counter++)
			{
				entry_list->push_back(make_shared<SCSEntry>(file_name, pos, access_mode, source_type));
				pos += 0x20;
			}
			return entry_list;
		}
		catch (ios::failure e)
		{
			return make_shared<_SCSEntryList>();
		}
	}

	SCSEntryList __stdcall zipToEntries(string file_name, uint16_t access_mode)
	{
		try
		{
			ifstream stream(file_name, ios::in | ios::binary);

			SCSEntryList list = make_shared<_SCSEntryList>();
			stream.seekg(-0x16L, ios::end);
			uint32_t sign = 0;
			while (stream.tellg())
			{
				stream.read((char*)&sign, 4);
				if (sign == 0x06054b50UL)
					break;
				stream.seekg(-5, ios::cur);
			}
			if (sign != 0x06054b50UL)return list;
			uint16_t fileCount;
			stream.seekg(4, ios::cur);
			stream.read((char*)&fileCount, 2);
			uint32_t entryPos;
			stream.seekg(6, ios::cur);
			stream.read((char*)&entryPos, 4);
			stream.seekg(entryPos, ios::beg);
			stream.close();
			for (uint16_t c = 0; c < fileCount; c++)
			{
				list->push_back(make_shared<SCSEntry>(file_name, entryPos, access_mode, SourceType::ZIP));
				stream.open(file_name, ios::in | ios::binary);
				if (!stream)exit(0);
				stream.seekg((size_t)entryPos + 0x1C, ios::beg);
				uint16_t nameLen, fieldLen, commLen;
				stream.read((char*)&nameLen, 0x2);
				stream.read((char*)&fieldLen, 0x2);
				stream.read((char*)&commLen, 0x2);
				entryPos += 0x2EUL + nameLen + fieldLen + commLen;
				stream.close();
			}
			if (access_mode & EntryMode::buildFolderContent)
				buildFolder(list);
			return list;
		}
		catch (ios::failure e)
		{
			return make_shared<_SCSEntryList>();
		}
	}

	SCSEntryList __stdcall folderToEntries(string root_name, uint16_t access_mode)
	{
		SCSEntryList list = make_shared<_SCSEntryList>();
		stdfs::path fileRoot(root_name);
		if (!stdfs::exists(root_name))throw("Invalid root");
		for (auto const& dir_entry : stdfs::recursive_directory_iterator(fileRoot))
			list->push_back(make_shared<SCSEntry>(dir_entry.path().string(), fileRoot.string(), access_mode));
		if (access_mode & EntryMode::buildFolderContent)
			buildFolder(list);
		return list;
	}

	SCSEntryList __stdcall fileToEntries(string file_name, uint16_t access_mode)
	{
		if(stdfs::is_directory(file_name))
			return folderToEntries(file_name, access_mode);
		uint32_t header;
		ifstream stream(file_name, ios::in | ios::binary);
		stream.read((char*)&header, 4);
		stream.close();
		if (header == 0x23534353)
			return scssToEntries(file_name, access_mode);
		return zipToEntries(file_name, access_mode);
	}



	SCSPathList __stdcall getListFromFile(string file_name)
	{
		SCSPathList list = make_shared<_SCSPathList>();
		try
		{
			ifstream stream(file_name, ios::in);
			string str;
			stream.seekg(0, ios::beg);
			while (!stream.eof())
			{
				getline(stream, str);
				list->push_back(str);
			}
			stream.close();
			sort(list->begin(), list->end());
			list->erase(unique(list->begin(), list->end()), list->end());
			return list;
		}
		catch (ios::failure e)
		{
			return list;
		}
	}

	SCSPathList __stdcall getListFromLog(string file_name)
	{
		try
		{
			ifstream file(file_name, ios::in | ios::binary);
			auto return_value = sfan::extractLogToList(file);
			file.close();
			return return_value;
		}
		catch (istream::failure e)
		{
			return make_shared<_SCSPathList>();
		}
	}

	SCSPathList __stdcall convertMapToList(SCSDictionary map)
	{
		SCSPathList list = make_shared<_SCSPathList>();
		for (auto& itr : *map)
			list->push_back(itr.second);
		sort(list->begin(), list->end());
		return list;
	}

	SCSPathList __stdcall entriesToList(SCSEntryList entry_list)
	{
		SCSPathList path_list = make_shared<_SCSPathList>();
		for (auto& itr : *entry_list)
		{
			if (itr->have_file_name)
			{
				path_list->push_back(itr->file_name);
				sort(path_list->begin(), path_list->end());
			}
			if (itr->have_path_list)
			{
				_SCSPathList teplst(*itr->path_list);
				teplst.erase(remove_if(teplst.begin(), teplst.end(), [](std::string s) {return s.at(0) == '~' || s.at(0) == '*'; }), teplst.end());
				move(teplst.begin(), teplst.end(), back_inserter(*path_list));
				sort(path_list->begin(), path_list->end());
				path_list->erase(unique(path_list->begin(), path_list->end()), path_list->end());
			}
		}
		return path_list;
	}

	SCSPathList __stdcall modFileToList(string file_name, SCSEntryList (*method)(string, uint16_t), uint16_t access_mode)
	{
		auto entries = method(file_name, access_mode);
		auto list = entriesToList(entries);
		dropEntryListContents(entries, [](auto) {return true; });
		return list;
	}



	SCSDictionary __stdcall getMapFromFile(string file_name)
	{
		SCSDictionary map = make_shared<_SCSDictionary>();
		ifstream stream(file_name, ios::in);
		try
		{
			stream.seekg(0, ios::beg);
			while (!stream.eof())
			{
				string str, hss;
				uint64_t hash;
				while (!stream.eof())
				{
					std::getline(stream, hss, ',');
					if (hss.size() == 0)
						break;
					std::getline(stream, str);
					hash = std::stoull(hss.c_str(), nullptr, 16);
				}
				map->insert(std::pair<uint64_t, string>(hash, str));
			}
			stream.close();
			return map;
		}
		catch (ios::failure e)
		{
			return map;
		}
	}

	SCSDictionary __stdcall getMapFromLog(string file_name)
	{
		return convertListToMap(getListFromLog(file_name));
	}

	SCSDictionary __stdcall convertListToMap(SCSPathList list)
	{
		SCSDictionary map = make_shared<_SCSDictionary>();
		for (auto itr : *list)
			map->insert(std::make_pair(getHash(itr), itr));
		return map;
	}

	SCSDictionary __stdcall entriesToMap(SCSEntryList entry_list)
	{
		return convertListToMap(entriesToList(entry_list));
	}

	SCSDictionary __stdcall modFileToMap(string file_name, SCSEntryList (*method)(string, uint16_t), uint16_t access_mode)
	{
		auto entries = method(file_name, access_mode);
		auto map = convertListToMap(entriesToList(entries));
		dropEntryListContents(entries, [](auto) {return true; });
		return map;
	}



	errno_t __stdcall saveListToFile(SCSPathList list, string file_name)
	{
		try
		{
			std::ofstream stream(file_name, ios::out);
			for (auto itr : *list)
				stream << itr << std::endl;
			stream.close();
			return 0;
		}
		catch (ios::failure e)
		{
			return -1;
		}
	}

	errno_t __stdcall saveMapToFile(SCSDictionary map, string file_name)
	{
		try
		{
			std::ofstream stream(file_name, ios::out);
			for (auto itr : *map)
				stream << std::hex << itr.first << ',' << itr.second << std::endl;
			stream.close();
			return 0;
		}
		catch (ios::failure e)
		{
			return -1;
		}
	}



	errno_t __stdcall entryToScss(SCSEntryList entry_list, string file_name)
	{
		std::ofstream stream(file_name, ios::out | ios::binary);
		char init[] = { 0x53, 0x43, 0x53, 0x23, 0x01, 0x00, 0x00, 0x00, 0x43, 0x49, 0x54, 0x59 };
		char zero[0xFF0] = { 0x00, 0x10 };
		uint32_t init_count = (uint32_t)entry_list->size();
		stream.write(init, 0xC);
		stream.write((char*)&init_count, 0x4);
		stream.write(zero, 0xFF0);
		uint64_t offset = 0x1000 + entry_list->size() * 0x20;
		sort(entry_list->begin(), entry_list->end(), [](pSCSEntry a, pSCSEntry b) {return a->hashcode < b->hashcode; });
		for (auto itr : *entry_list)
		{
			if (itr->compressed_type == CompressedType::ZIP)
			{
				itr->inflateContent();
				itr->deflateContent(CompressedType::SCS);
			}
			else
				itr->receiveContent();
			uint64_t hashcode = itr->hashcode;
			stream.write((char*)&hashcode, 0x08);
			stream.write((char*)&offset, 0x08);
			uint32_t save_type = 0; //if ZIP
			save_type += ((itr->compressed_type==CompressedType::SCS) * 2);
			save_type += (itr->getFileType() == FileType::folder);
			stream.write((char*)&save_type, 0x4);
			uint32_t crc = itr->getCrc();
			stream.write((char*)&crc, 0x4);
			auto size = itr->decrypted ? itr->uncompressed_size - 6 : itr->uncompressed_size;
			uint32_t zsize = itr->output_size;
			stream.write((char*)&size, 0x4);
			stream.write((char*)&zsize, 0x4);
			offset += zsize;
		}
		for (auto itr : *entry_list)
		{
			uint32_t zsize = itr->output_size;
			Buff buff(new char[zsize]);
			auto p = itr->content;
			p->seekg(0, ios::beg);
			p->read(buff.get(), zsize);
			stream.write(buff.get(), zsize);
		}
		stream.close();
		return true;
	}

	errno_t __stdcall entryToZip(SCSEntryList entry_list, string file_name)
	{
		std::ofstream stream1(file_name, ios::out | ios::binary);
		stringstream stream2(ios::in | ios::out | ios::binary);
		uint16_t counter = 0;
		char zero[16] = { 0 };
		for (auto itr : *entry_list)
		{
			if (!itr->have_file_name || itr->file_name.size() == 0)continue;
			counter += 1;
			if (itr->compressed_type == CompressedType::SCS)
			{
				itr->inflateContent();
				itr->deflateContent(CompressedType::ZIP);
			}
			else
				itr->receiveContent();
			uint32_t offset = (uint32_t)stream1.tellp();
			char h[] = { 0x50,0x4B,0x03,0x04,0x14,0x00,0x00,0x00 };
			stream1.write(h, 8);
			uint16_t com = itr->compressed_type == CompressedType::ZIP ? 8 : 0;
			stream1.write((char*)&com, 2);
			stream1.write(zero, 4);
			auto crc = (itr->save_type == SaveType::ComFolder || itr->save_type == SaveType::UncomFolder) ? 0 : itr->calcCrc();
			stream1.write((char*)&crc, 4);
			uint32_t nameLen = (uint32_t)itr->file_name.size();
			string name(itr->file_name);
			auto size = itr->decrypted?itr->uncompressed_size-6: itr->uncompressed_size;
			auto zsize = crc ? itr->output_size:0;
			if (itr->save_type == SaveType::ComFolder || itr->save_type == SaveType::UncomFolder)
			{
				name += "/";
				nameLen += 1;
				size = zsize = 0;
			}
			stream1.write((char*)&zsize, 4);
			stream1.write((char*)&size, 4);
			stream1.write((char*)&nameLen, 2);
			stream1.write(zero, 2);
			stream1.write(name.c_str(), nameLen);
			if (itr->save_type != SaveType::ComFolder && itr->save_type != SaveType::UncomFolder)
			{
				Buff buff(new char[zsize]);
				auto p = itr->content;
				p->seekg(0, ios::beg);
				p->read(buff.get(), zsize);
				stream1.write(buff.get(), zsize);
			}
			char ch[] = { 0x50,0x4B,0x01,0x02,0x14,0x00,0x14,0x00,0x00,0x00 };
			stream2.write(ch, 10);
			stream2.write((char*)&com, 2);
			stream2.write(zero, 4);
			stream2.write((char*)&crc, 4);
			stream2.write((char*)&zsize, 4);
			stream2.write((char*)&size, 4);
			stream2.write((char*)&nameLen, 2);
			stream2.write(zero, 8);
			uint32_t exa = (itr->save_type == SaveType::ComFolder || itr->save_type == SaveType::UncomFolder) ? 0x10 : 0x20;
			stream2.write((char*)&exa, 4);
			stream2.write((char*)&offset, 4);
			stream2.write(name.c_str(), nameLen);

		}
		uint32_t size = (uint32_t)stream2.tellp();
		uint32_t ch_start = (uint32_t)stream1.tellp();
		stream2.seekg(0, ios::beg);
		Buff buff(new char[size]);
		stream2.read(buff.get(), size);
		stream1.write(buff.get(), size);
		char eh[] = { 0x50,0x4B,0x05,0x06,0x00,0x00,0x00,0x00 };
		stream1.write(eh, 8);
		stream1.write((char*)&counter, 2);
		stream1.write((char*)&counter, 2);
		stream1.write((char*)&size, 4);
		stream1.write((char*)&ch_start, 4);
		stream1.write(zero, 2);
		stream1.close();
		return true;
	}

	errno_t __stdcall entryToFolder(SCSEntryList entry_list, string root_name)
	{
		stdfs::path fileRoot(root_name);
		stdfs::create_directories(root_name);
		if (!stdfs::exists(root_name))throw("Invalid root");
		for (auto itr : *entry_list)
		{
			string file_name;
			if (itr->haveFileName())
				file_name = itr->getFileName();
			else
				continue;
			while (file_name.find_first_of('/') != string::npos)
				file_name.replace(file_name.find_first_of("/"), 1, "\\");
			std::filesystem::path folder;
			if (itr->getFileType() == FileType::folder)
			{
				folder = root_name + "\\" + file_name;
				stdfs::create_directories(folder);
				continue;
			}
			if (file_name.find_first_of("\\") != string::npos)
			{
				folder = root_name + "\\" + file_name.substr(0, file_name.find_last_of("\\"));
				stdfs::create_directories(folder);
			}
			SCSContent p = nullptr;
			if (itr->file_type == FileType::sii3nK || itr->file_type == FileType::siiEncrypted || itr->file_type == FileType::siiBinary)
				p = itr->decryptedSII();
			else
				p = itr->inflatedContent();
			std::ofstream output(root_name + "\\" + file_name, ios::out | ios::binary);
			p->seekg(0, ios::end);
			size_t buffSize = p->tellg();
			Buff buff(new char[buffSize]);
			p->seekg(0, ios::beg);
			p->read(buff.get(), buffSize);
			output.write(buff.get(), buffSize);
			output.close();
			itr->dropContent();
		}
		return 0;
	}



	size_t __stdcall getResolvedFolderCount(SCSEntryList entry_list)
	{
		size_t count = 0;
		for (auto itr : *entry_list)
			if (itr->haveFileName() && itr->getFileType() == FileType::folder)
				++count;
		return count;
	}

	size_t __stdcall getResolvedDirCount(SCSEntryList entry_list)
	{
		size_t count = 0;
		for (auto& itr : *entry_list)
			if (itr->haveFileName() && itr->getFileType() != FileType::folder)
				++count;
		return count;
	}

	size_t __stdcall getResolvedFileCount(SCSEntryList entry_list)
	{
		size_t count = 0;
		for (auto itr : *entry_list)
			if (itr->haveFileName())
				++count;
		return count;
	}



	size_t __stdcall analyzeEntries(SCSEntryList entry_list)
	{
		return analyzeEntriesWithMap(entry_list, make_shared<_SCSDictionary>());
	}

	size_t __stdcall analyzeEntriesWithMap(SCSEntryList entry_list, SCSDictionary map)
	{
		_SCSPathList init_list({ "","manifest.sii","automat","contentbrowser","custom" ,"def" ,"dlc" ,"effect" ,"font" ,"map" ,"material" ,"model" ,
			"model2" ,"prefab" ,"prefab2" ,"road_template" ,"sound" ,"system" ,"ui" ,"unit" ,"vehicle" ,"video" ,"autoexec" ,"version", "cfg", "locale" });
		for (auto itr : init_list)
			map->insert(std::pair<uint64_t, string>(getHash(itr), itr));
		size_t filecount;
		bool FurtherIndex;
		do
		{
			FurtherIndex = false;
			filecount = getResolvedFileCount(entry_list);
			SCSPathList _path_list = entriesToList(entry_list);
			for (auto itr : *_path_list)
			{
				auto itr2 = map->find(getHash(itr));
				map->insert(std::pair<uint64_t, string>(getHash(itr), itr));
			}
			for (auto itr : *entry_list)
			{
				if (!itr->haveFileName())
				{
					itr->searchName(map);
					if (itr->haveFileName())
					{
						if (itr->getFileName().size() > 3)
							if (itr->getFileName().substr(itr->getFileName().size() - 3).compare("sui") == 0 && !itr->pass)
							{
								if (!itr->haveContent())
								{
									itr->receiveContent();
									itr->inflateContent();
									itr->setPathList(sfan::extractTextToList(*itr->receivedContent()));
									itr->dropContent();
								}
								else
									itr->setPathList(sfan::extractTextToList(*itr->receivedContent()));
							}
						if (itr->getFileName().size() > 8)
							if (itr->getFileName().substr(itr->getFileName().size() - 8).compare("soundref") == 0 && !itr->pass)
							{
								if (!itr->haveContent())
								{
									itr->receiveContent();
									itr->inflateContent();
									itr->setPathList(sfan::extractSoundrefToList(*itr->receivedContent()));
									itr->dropContent();
								}
								else
									itr->setPathList(sfan::extractTextToList(*itr->receivedContent()));
							}
						itr->toAbsPath();
					}
				}
			}
			if (filecount < getResolvedFileCount(entry_list))
				FurtherIndex = true;
		} while (FurtherIndex);
		return filecount;
	}



	uint32_t __stdcall deflateEntryList(SCSEntryList entry_list, bool (*filter)(pSCSEntry), CompressedType compressed_type)
	{
		uint32_t counter = 0;
		for (auto& itr : *entry_list)
			if (filter(itr))
			{
				itr->deflateContent(compressed_type);
				counter++;
			}
		return counter;
	}

	uint32_t __stdcall inflateEntryList(SCSEntryList entry_list, bool (*filter)(pSCSEntry))
	{
		uint32_t counter = 0;
		for (auto itr : *entry_list)
			if (filter(itr))
			{
				itr->inflateContent();
				counter++;
			}
		return counter;
	}

	uint32_t __stdcall decryptEntryList(SCSEntryList entry_list, bool (*filter)(pSCSEntry))
	{
		uint32_t counter = 0;
		for (auto itr : *entry_list)
			if (filter(itr))
			{
				itr->decryptSII();
				counter++;
			}
		return counter;
	}

	uint32_t __stdcall encryptEntryList(SCSEntryList entry_list, bool (*filter)(pSCSEntry))
	{
		uint32_t counter = 0;
		for (auto itr : *entry_list)
			if (filter(itr))
			{
				itr->encryptSII();
				counter++;
			}
		return counter;
	}

	uint32_t __stdcall receiveEntryListContents(SCSEntryList entry_list, bool (*filter)(pSCSEntry))
	{
		uint32_t counter = 0;
		for (auto itr : *entry_list)
			if (filter(itr))
			{
				itr->receiveContent();
				counter++;
			}
		return counter;
	}

	uint32_t __stdcall dropEntryListContents(SCSEntryList entry_list, bool (*filter)(pSCSEntry))
	{
		uint32_t counter = 0;
		for (auto itr : *entry_list)
			if (filter(itr))
			{
				itr->dropContent();
				counter++;
			}
		return counter;
	}



	void __stdcall buildFolder(SCSEntryList entry_list)
	{
		for (auto itr1 : *entry_list)
		{
			if (itr1->file_name.size() == 0)continue;
			pSCSEntry entry = nullptr;
			auto folderName = itr1->file_name.substr(0, itr1->file_name.find_last_of('/') == string::npos ? 0 : itr1->file_name.find_last_of('/'));
			for (auto itr2 : *entry_list)
			{
				if (itr2->hashcode == getHash(folderName))
				{
					entry = itr2;
					if (!itr2->have_path_list)
						itr2->path_list = make_shared<_SCSPathList>();
					for (auto itr3 : *itr2->path_list)
						if (itr3.compare(itr1->file_name) == 0 && itr3.find(itr1->file_name) != string::npos)
							return;
					if (itr2->have_content)
					{
						auto _content = itr2->inflateContent();
						_content->seekp(0, ios::end);
					}
					else
						itr2->content = make_shared<stringstream>(ios::in | ios::out | ios::binary);
				}
			}
			if (entry == nullptr)
			{
				entry = make_shared<SCSEntry>(
					true, true, false, false, 0, 0, 0, 0, 0, CompressedType::Uncompressed,false, SaveType::UncomFolder, 0, SourceType::NoSource, "",
					FileType::folder, make_shared<stringstream>(ios::in|ios::out|ios::binary), folderName, make_shared<_SCSPathList>());
				entry_list->push_back(entry);
			}
			auto lName = itr1->file_name.substr(itr1->file_name.find_last_of('/') + 1);
			if (itr1->getSaveType() == SaveType::UncomFolder || itr1->getSaveType() == SaveType::ComFolder)
				lName = "*" + lName;
			if (entry->output_size != 0)
			{
				char nl = '\n';
				entry->receivedContent()->write(&nl, 1);
				entry->output_size += 1;
			}
			entry->receivedContent()->write(lName.c_str(), lName.size());
			entry->output_size += (uint32_t)lName.size();
			entry->uncompressed_size = entry->output_size;
			entry->source_type = SourceType::NoSource;
			entry->calcCrc();
		}
	}

}