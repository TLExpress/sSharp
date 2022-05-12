#include "scsFileAccess.hpp"

namespace scsFileAccess
{

	SCSEntryList scssToEntries(string file_name, uint16_t access_mode)
	{
		SCSEntryList entry_list = make_shared<_SCSEntryList>();
		try
		{
			ifstream stream(file_name, ios::in | ios::binary);
			if (!stream)
				throw SCSSException(__func__, "file is unavailable");

			uint32_t sign;
			uint32_t entryCount;

			if(sfan::getStreamSize(stream)<4)
				throw SCSSException(__func__, "invalid file format");
			stream.seekg(0, ios::beg).read((char*)&sign, 4);
			if (sign != 0x23534353)
				throw SCSSException(__func__, "invalid file format");

			stream.seekg(0xC, ios::beg).read((char*)&entryCount, sizeof(entryCount));
			stream.close();

			size_t pos = 0x1000;
			for (uint32_t counter = 0; counter < entryCount; counter++, pos+=0x20)
				entry_list->push_back(make_shared<SCSEntry>(file_name, pos, access_mode, SourceType::SCS));

			if (access_mode & EntryMode::analyze)
				analyzeEntries(entry_list);

		}
		catch (SCSSException) {}
		catch (...) { sfan::ueMessage(__func__); }
		return entry_list;
	}

	SCSEntryList zipToEntries(string file_name, uint16_t access_mode)
	{
		SCSEntryList entry_list = make_shared<_SCSEntryList>();
		try
		{
			ifstream stream(file_name, ios::in | ios::binary);
			if (!stream || stream.seekg(0, ios::end).tellg()<16)
				throw SCSSException(__func__, "file is unavailable");

			stream.seekg(-0x16L, ios::end);

			uint32_t sign=0;
			while (stream.tellg())
			{
				stream.read((char*)&sign, 4);
				if (sign == 0x06054b50UL)
					break;
				stream.seekg(-5, ios::cur);
			}
			if (sign != 0x06054b50UL)
				throw SCSSException(__func__, "invalid file format");

			uint16_t fileCount;
			uint32_t entryPos;
			stream.seekg(4, ios::cur).read((char*)&fileCount, 2);
			stream.seekg(6, ios::cur).read((char*)&entryPos, 4);
			stream.close();

			for (uint16_t c = 0; c < fileCount; c++)
			{
				entry_list->push_back(make_shared<SCSEntry>(file_name, entryPos, access_mode, SourceType::ZIP));
				stream.open(file_name, ios::in | ios::binary);
				if (!stream)
					throw SCSSException(__func__, "file is unavailable");

				uint16_t nameLen, fieldLen, commLen;
				stream.seekg((size_t)entryPos + 0x1C, ios::beg).read((char*)&nameLen, 0x2).read((char*)&fieldLen, 0x2).read((char*)&commLen, 0x2);
				entryPos += 0x2EUL + nameLen + fieldLen + commLen;
				stream.close();
			}
			if (access_mode & EntryMode::buildFolderContent)
				buildFolder(entry_list);
		}
		catch (SCSSException) {}
		catch (...) { sfan::ueMessage(__func__); }
		return entry_list;
	}

	SCSEntryList folderToEntries(string root_name, uint16_t access_mode)
	{
		SCSEntryList entry_list = make_shared<_SCSEntryList>();
		try
		{
			stdfs::path fileRoot(root_name);
			if (!stdfs::exists(root_name))
				throw SCSSException(__func__, "invalid root");
			for (auto const& dir_entry : stdfs::recursive_directory_iterator(fileRoot))
				entry_list->push_back(make_shared<SCSEntry>(dir_entry.path().string(), fileRoot.string(), access_mode));
			if (access_mode & EntryMode::buildFolderContent)
				buildFolder(entry_list);
		}
		catch (SCSSException) {}
		catch (...) { sfan::ueMessage(__func__); }
		return entry_list;
	}

	SCSEntryList fileToEntries(string file_name, uint16_t access_mode)
	{
		try
		{
			if (!stdfs::exists(file_name))
				throw SCSSException(__func__, "file is not exist");

			if (stdfs::is_directory(file_name))
				return folderToEntries(file_name, access_mode);

			ifstream stream(file_name, ios::in | ios::binary);
			if (!stream || stream.seekg(0,ios::end).tellg()<4)
				throw SCSSException(__func__, "file is unavailable");

			uint32_t header;
			stream.seekg(0,ios::beg).read((char*)&header, 4);
			stream.close();
			if (header == 0x23534353)
				return scssToEntries(file_name, access_mode);
			return zipToEntries(file_name, access_mode | EntryMode::identByFileName);
		}
		catch (SCSSException) {}
		catch (...) { sfan::ueMessage(__func__); }
		return make_shared<_SCSEntryList>();
	}



	SCSPathList getListFromFile(string file_name)
	{
		SCSPathList list = make_shared<_SCSPathList>();
		try
		{
			ifstream stream(file_name, ios::in);
			if (!stream)
				throw SCSSException(__func__, "file is unavailable");
			while (!stream.eof())
			{
				if (!stream)
					throw SCSSException(__func__, "stream failed");
				string str;
				getline(stream, str);
				list->push_back(str);
			}
		}
		catch (SCSSException) {}
		catch (...) { sfan::ueMessage(__func__); }
		list->sort();
		list->unique();
		return list;
	}

	SCSPathList getListFromLog(string file_name)
	{
		try
		{
			ifstream file(file_name, ios::in | ios::binary);
			if (!file)
				throw SCSSException(__func__, "file is unavailable");
			return sfan::extractLogToList(file);
		}
		catch (SCSSException ) {}
		catch (...) { sfan::ueMessage(__func__); }
		return make_shared<_SCSPathList>();
	}

	SCSPathList convertMapToList(SCSDictionary map)
	{
		SCSPathList list = make_shared<_SCSPathList>();
		for (auto& itr : *map)
			list->push_back(itr.second);
		list->sort();
		return list;
	}

	SCSPathList entriesToList(SCSEntryList entry_list)
	{
		SCSPathList path_list = make_shared<_SCSPathList>();
		for (auto const& itr : *entry_list)
			if (itr->have_path_list)
			{
				_SCSPathList teplst(*itr->path_list);
				teplst.remove_if([](string s) {return !s.empty()&&(s.at(0) == '~' || s.at(0) == '*'); });
				move(teplst.begin(), teplst.end(), back_inserter(*path_list));
			}
		path_list->sort();
		path_list->unique();
		return path_list;
	}

	SCSPathList modFileToList(string file_name, function<SCSEntryList(string, uint16_t)> method, uint16_t access_mode)
	{
		auto entries = method(file_name, access_mode);
		return entriesToList(entries);
	}



	SCSDictionary getMapFromFile(string file_name)
	{
		SCSDictionary map = make_shared<_SCSDictionary>();
		try
		{
			ifstream stream(file_name, ios::in);
			if (!stream)
				throw SCSSException(__func__, "file is unavailable");
			while (!stream.eof())
			{
				string str, hss;
				uint64_t hash;
				while (!stream.eof())
				{
					if (!stream)
						throw SCSSException(__func__, "stream failed");
					std::getline(stream, hss, ',');
					if (stream.eof())
						break;
					std::getline(stream, str);
					hash = std::stoull(hss.c_str(), nullptr, 16);
				}
				map->insert(std::pair<uint64_t, string>(hash, str));
			}
		}
		catch (SCSSException) {}
		catch (...) { sfan::ueMessage(__func__); }
		return map;
	}

	SCSDictionary getMapFromLog(string file_name)
	{
		return convertListToMap(getListFromLog(file_name));
	}

	SCSDictionary convertListToMap(SCSPathList list)
	{
		SCSDictionary map = make_shared<_SCSDictionary>();
		for (auto& itr : *list)
			map->insert(make_pair(getHash(itr), itr));
		return map;
	}

	SCSDictionary entriesToMap(SCSEntryList entry_list)
	{
		return convertListToMap(entriesToList(entry_list));
	}

	SCSDictionary modFileToMap(string file_name, function<SCSEntryList(string, uint16_t)> method, uint16_t access_mode)
	{
		auto entries = method(file_name, access_mode);
		return convertListToMap(entriesToList(entries));
	}



	errno_t saveListToFile(SCSPathList list, string file_name)
	{
		try
		{
			std::ofstream stream(file_name, ios::out);
			if (!stream)
				throw SCSSException(__func__, "file is unavailable");
			for (auto const& itr : *list)
				stream << itr << std::endl;
			return 0;
		}
		catch (SCSSException e) {}
		catch (...) { sfan::ueMessage(__func__); }
		return -1;
	}

	errno_t saveMapToFile(SCSDictionary map, string file_name)
	{
		try
		{
			std::ofstream stream(file_name, ios::out);
			if (!stream)
				throw SCSSException(__func__, "file is unavailable");
			for (auto const& itr : *map)
				stream << std::hex << itr.first << ',' << itr.second << std::endl;
			return 0;
		}
		catch (SCSSException e) {}
		catch (...) { sfan::ueMessage(__func__); }
		return -1;
	}



	errno_t entriesToScss(SCSEntryList entry_list, string file_name)
	{
		return entriesToScss(entry_list, file_name, [](auto) {return true; });
	}

	errno_t entriesToZip(SCSEntryList entry_list, string file_name)
	{
		return entriesToZip(entry_list, file_name, [](auto) {return true; });
	}

	errno_t entriesToFolder(SCSEntryList entry_list, string file_name)
	{
		return entriesToFolder(entry_list, file_name, [](auto) {return true; });
	}

	errno_t entriesToScss(SCSEntryList entry_list, string file_name, function<bool(pSCSEntry)> filter)
	{
		try
		{
			std::ofstream stream(file_name, ios::out | ios::binary);
			if (!stream)
				throw SCSSException(__func__, "file is unavailable");
			const char init[] = { 0x53, 0x43, 0x53, 0x23, 0x01, 0x00, 0x00, 0x00, 0x43, 0x49, 0x54, 0x59 };
			const char zero[0xFF0] = { 0x00, 0x10 };

			uint32_t init_count=0;
			for (auto const& itr : *entry_list)
			{
				if (filter(itr))
					init_count++;
			}

			stream.write(init, 0xC).write((char*)&init_count, 0x4).write(zero, 0xFF0);

			sort(entry_list->begin(), entry_list->end(), [](pSCSEntry a, pSCSEntry b) {return a->hashcode < b->hashcode; });
			uint64_t offset = 0x1000ULL + (uint64_t)init_count * 0x20ULL;
			for (auto const& itr : *entry_list)
			{
				if (!filter(itr))
					continue;
				if (!stream)
					throw SCSSException(__func__, "stream failed");

				if (itr->compressed_type == CompressedType::ZIP)
				{
					itr->inflateContent();
					itr->deflateContent(CompressedType::SCS);
				}
				else
					itr->receiveContent();

				uint64_t hashcode = itr->hashcode;
				stream.write((char*)&hashcode, 0x08).write((char*)&offset, 0x08);

				uint32_t save_type = 0; 
				save_type += ((itr->compressed_type == CompressedType::SCS) * 2);
				save_type += (itr->getFileType() == FileType::folder);
				stream.write((char*)&save_type, 0x4);

				uint32_t crc = itr->getCrc();
				stream.write((char*)&crc, 0x4);

				auto size = itr->decrypted ? itr->uncompressed_size - 6 : itr->uncompressed_size;
				uint32_t zsize = itr->output_size;
				stream.write((char*)&size, 0x4).write((char*)&zsize, 0x4);
				offset += zsize;
			}
			for (auto const& itr : *entry_list)
			{
				if (!filter(itr))
					continue;
				if (!stream)
					throw SCSSException(__func__, "stream failed");

				uint32_t zsize = itr->output_size;
				Buff buff(new char[zsize]);
				auto p = itr->content;
				p->seekg(0, ios::beg).read(buff.get(), zsize);
				stream.write(buff.get(), zsize);
			}
			return 0;
		}
		catch (SCSSException) {}
		catch (...) { sfan::ueMessage(__func__); }
		return -1;
	}

	errno_t entriesToZip(SCSEntryList entry_list, string file_name, function<bool(pSCSEntry)> filter)
	{
		try
		{
			std::ofstream stream1(file_name, ios::out | ios::binary);
			if (!stream1)
				throw SCSSException(__func__, "file is unavailable");
			stringstream stream2(ios::in | ios::out | ios::binary);

			uint16_t counter = 0;
			const char zero[16] = { 0 };
			for (auto itr : *entry_list)
			{
				if (!filter(itr)|| itr->have_file_name && itr->file_name.empty())
					continue;
				if (!stream1 || !stream2)
					throw SCSSException(__func__, "stream failed");
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
				uint16_t com = itr->compressed_type == CompressedType::ZIP ? 8 : 0;
				auto crc = (itr->save_type == SaveType::ComFolder || itr->save_type == SaveType::UncomFolder) ? 0 : itr->calcCrc();
				string name;

				auto size = itr->decrypted ? itr->uncompressed_size - 6 : itr->uncompressed_size;
				auto zsize = crc||!itr->have_file_name ? itr->output_size : 0;
				SaveType save_type;
				if (itr->have_file_name)
				{
					name = itr->file_name;
					if (itr->save_type == SaveType::ComFolder || itr->save_type == SaveType::UncomFolder)
					{
						name += "/";
						size = zsize = 0;
					}
					save_type = itr->save_type;
				}
				else
				{
					stringstream name_stream;
					name_stream << "_unresolved_/CITY(" <<std::setw(16)<<std::setfill('0') << std::hex << itr->hashcode << ")" << ((itr->save_type == SaveType::ComFolder || itr->save_type == SaveType::UncomFolder) ? "D" : "F");
					name = name_stream.str();
					save_type = itr->compressed_type == CompressedType::Uncompressed ? SaveType::UncomFile : SaveType::ComFile;
				}
				uint32_t nameLen = (uint32_t)name.size();

				stream1.write(h, 8).write((char*)&com, 2).write(zero, 4).write((char*)&crc, 4).write((char*)&zsize, 4).write((char*)&size, 4);
				stream1.write((char*)&nameLen, 2).write(zero, 2).write(name.c_str(), nameLen);

				if (save_type != SaveType::ComFolder && save_type != SaveType::UncomFolder)
				{
					Buff buff(new char[zsize]);
					auto p = itr->content;
					p->seekg(0, ios::beg);
					p->read(buff.get(), zsize);
					stream1.write(buff.get(), zsize);
				}

				char ch[] = { 0x50,0x4B,0x01,0x02,0x14,0x00,0x14,0x00,0x00,0x00 };
				uint32_t exa = (save_type == SaveType::ComFolder || save_type == SaveType::UncomFolder) ? 0x10 : 0x20;

				stream2.write(ch, 10).write((char*)&com, 2).write(zero, 4).write((char*)&crc, 4).write((char*)&zsize, 4).write((char*)&size, 4);
				stream2.write((char*)&nameLen, 2).write(zero, 8).write((char*)&exa, 4).write((char*)&offset, 4).write(name.c_str(), nameLen);
			}

			if (!stream1 || !stream2)
				throw SCSSException(__func__, "stream failed");

			uint32_t size = (uint32_t)stream2.tellp();
			uint32_t ch_start = (uint32_t)stream1.tellp();
			Buff buff(new char[size]);
			const char eh[] = { 0x50,0x4B,0x05,0x06,0x00,0x00,0x00,0x00 };

			stream2.seekg(0, ios::beg).read(buff.get(), size);
			stream1.write(buff.get(), size).write(eh, 8).write((char*)&counter, 2).write((char*)&counter, 2);
			stream1.write((char*)&size, 4).write((char*)&ch_start, 4).write(zero, 2);
			return 0;
		}
		catch (SCSSException) {}
		catch (...) { sfan::ueMessage(__func__); }
		return -1;
	}

	errno_t entriesToFolder(SCSEntryList entry_list, string root_name, function<bool(pSCSEntry)> filter)
	{
		try
		{
			stdfs::path fileRoot(root_name);
			stdfs::create_directories(root_name);
			if (!stdfs::exists(root_name))
				throw SCSSException(__func__, "invalid root");
			for (auto const& itr : *entry_list)
			{
				if (!filter(itr)|| itr->haveFileName() && itr->file_name.empty())
					continue;
				string file_name;
				if (itr->have_file_name)
					file_name = itr->file_name;
				else
				{
					stringstream name_stream;
					name_stream << "_unresolved_/CITY(" << std::setw(16) << std::setfill('0') << std::hex << itr->hashcode << ")" << ((itr->save_type == SaveType::ComFolder || itr->save_type == SaveType::UncomFolder) ? "D" : "F");
					file_name = name_stream.str();
				}

				while (file_name.find_first_of('/') != string::npos)
					file_name.replace(file_name.find_first_of("/"), 1, "\\");

				stdfs::path folder;
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
				auto p = itr->inflatedContent();

				if (!*p)
					throw SCSSException(__func__, "stream failed");

				std::ofstream output(root_name + "\\" + file_name, ios::out | ios::binary);
				if (!output)
					throw SCSSException(__func__, "file is unavailable");
				size_t buffSize = p->seekg(0, ios::end).tellg();
				Buff buff(new char[buffSize]);
				p->seekg(0, ios::beg).read(buff.get(), buffSize);
				output.write(buff.get(), buffSize);
			}
			return 0;
		}
		catch (SCSSException) {}
		catch (...) { sfan::ueMessage(__func__); }
		return -1;
		
	}



	size_t getResolvedFolderCount(SCSEntryList entry_list)
	{
		size_t count = 0;
		for (auto const& itr : *entry_list)
			if (itr->haveFileName() && itr->getFileType() == FileType::folder)
				++count;
		return count;
	}

	size_t getResolvedDirCount(SCSEntryList entry_list)
	{
		size_t count = 0;
		for (auto const& itr : *entry_list)
			if (itr->haveFileName() && itr->getFileType() != FileType::folder)
				++count;
		return count;
	}

	size_t getResolvedFileCount(SCSEntryList entry_list)
	{
		size_t count = 0;
		for (auto const& itr : *entry_list)
			if (itr->haveFileName())
				++count;
		return count;
	}



	size_t analyzeEntries(SCSEntryList entry_list)
	{
		return analyzeEntriesWithMap(entry_list, make_shared<_SCSDictionary>());
	}

	size_t analyzeEntriesWithMap(SCSEntryList entry_list, SCSDictionary map)
	{
		_SCSPathList init_list({ "","manifest.sii","automat","contentbrowser","custom" ,"def" ,"dlc" ,"effect" ,"font" ,"map" ,"material" ,"model" ,
			"model2" ,"prefab" ,"prefab2" ,"panorama" ,"road_template" ,"sound" ,"system" ,"ui","uilab" ,"unit" ,"vehicle" ,"video" ,"autoexec" ,"version", "cfg", "locale"});

		for (auto const& itr : init_list)
			map->insert(std::pair<uint64_t, string>(getHash(itr), itr));
		size_t filecount;
		bool FurtherIndex;
		do
		{
			FurtherIndex = false;
			filecount = getResolvedFileCount(entry_list);
			SCSPathList _path_list = entriesToList(entry_list);
			for (auto const& itr : *_path_list)
			{
				map->insert(std::pair<uint64_t, string>(getHash(itr), itr));
			}
			for (auto& itr : *entry_list)
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
									itr->setPathList(sfan::extractTextToList(*itr->decryptedSII()));
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



	uint32_t deflateEntryList(SCSEntryList entry_list, function<bool(pSCSEntry)> filter, CompressedType compressed_type)
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

	uint32_t inflateEntryList(SCSEntryList entry_list, function<bool(pSCSEntry)> filter)
	{
		uint32_t counter = 0;
		for (auto& itr : *entry_list)
			if (filter(itr))
			{
				itr->inflateContent();
				counter++;
			}
		return counter;
	}

	uint32_t decryptEntryList(SCSEntryList entry_list, function<bool(pSCSEntry)> filter)
	{
		uint32_t counter = 0;
		for (auto& itr : *entry_list)
			if (filter(itr))
			{
				itr->decryptSII();
				counter++;
			}
		return counter;
	}

	uint32_t decryptEntryList(SCSEntryList entry_list)
	{ return decryptEntryList(entry_list, [](auto) {return true; }); }

	uint32_t encryptEntryList(SCSEntryList entry_list, function<bool(pSCSEntry)> filter)
	{
		uint32_t counter = 0;
		for (auto& itr : *entry_list)
			if (filter(itr))
			{
				itr->encryptSII();
				counter++;
			}
		return counter;
	}

	uint32_t receiveEntryListContents(SCSEntryList entry_list, function<bool(pSCSEntry)> filter)
	{
		uint32_t counter = 0;
		for (auto& itr : *entry_list)
			if (filter(itr))
			{
				itr->receiveContent();
				counter++;
			}
		return counter;
	}

	uint32_t dropEntryListContents(SCSEntryList entry_list, function<bool(pSCSEntry)> filter)
	{
		uint32_t counter = 0;
		for (auto& itr : *entry_list)
			if (filter(itr))
			{
				itr->dropContent();
				counter++;
			}
		return counter;
	}



	void buildFolder(SCSEntryList entry_list)
	{
		auto extra = _SCSEntryList();
		for (auto const& itr1 : *entry_list)
		{
			if (itr1->file_name.size() == 0)
				continue;
			pSCSEntry entry = nullptr;
			auto folderName = itr1->file_name.substr(0, itr1->file_name.find_last_of('/') == string::npos ? 0 : itr1->file_name.find_last_of('/'));
			for (auto& itr2 : *entry_list)
			{
				if (itr2->hashcode == getHash(folderName))
				{
					entry = itr2;
					if (!itr2->have_path_list)
						itr2->path_list = make_shared<_SCSPathList>();
					auto q = itr2->path_list;
					for (auto const& itr3 : *q)
						if (itr3.compare(itr1->file_name) == 0 && itr3.find(itr1->file_name) != string::npos)
							return;
					if (itr2->have_content && itr2->uncompressed_size >0&&!(itr2->file_name.find("_unresolved_")==0&&itr2->file_name[itr2->file_name.size()-1]=='D'))
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
					true, true, true, false, 0, 0, 0, 0, 0, CompressedType::Uncompressed,false, SaveType::UncomFolder, 0, SourceType::NoSource, "",
					FileType::folder, make_shared<stringstream>(ios::in|ios::out|ios::binary), folderName, make_shared<_SCSPathList>());
				extra.push_back(entry);
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
			entry->path_list->push_back(lName);
			entry->output_size += (uint32_t)lName.size();
			entry->uncompressed_size = entry->output_size;
			entry->source_type = SourceType::NoSource;
			if(!(entry->file_name.find("_unresolved_") == 0 && entry->file_name[entry->file_name.size() - 1] == 'D'))
				entry->calcCrc();
		}
		move(extra.begin(), extra.end(), back_inserter(*entry_list));
	}

	void removeIndexes(SCSEntryList entry_list)
	{
		entry_list->sort([](pSCSEntry l, pSCSEntry r) {return l->file_name > r->file_name; });
		for (auto& itr : *entry_list)
			if (itr->file_type == FileType::folder && itr->have_path_list)
			{
				itr->path_list->remove_if([](auto itr2) {return itr2.size() < 3 || itr2.substr(itr2.size() - 3) != "sii" && itr2[0] != '*'; });
				itr->dropContentForce();
				itr->setContent(make_shared<stringstream>(ios::in|ios::out|ios::binary));
				for (auto& itr2 : *itr->path_list)
					*itr->content << itr2 << endl;
				itr->uncompressed_size = itr->content->tellp();
				itr->output_size = itr->content->tellp();
				itr->source_type = SourceType::NoSource;
				itr->deflateContent(CompressedType::SCS);
				itr->compressed_type = CompressedType::SCS;
			}
		entry_list->remove_if([](auto itr) {return itr->file_type == FileType::folder && (!itr->have_path_list || itr->path_list->empty()); });
	}
}