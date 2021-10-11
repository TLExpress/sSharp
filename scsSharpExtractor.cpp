#include "scsSharpExtractor.hpp"

int main(int argc, char** argv)
{
	if (argc <= 1)
	{
		std::cout << "No files." << std::endl;
	}

	std::map<uint64_t, std::string>* defmap = folderToMap("E:\\SCS\\141");
	mapToFile(defmap, "C:\\Users\\hank2\\Desktop\\00.csv");
	defmap->insert(std::pair<uint64_t, std::string>(0x9ae16a3b2f90404f, ""));
	defmap->insert(std::pair<uint64_t, std::string>(0xb97fff7ce7377c95, "manifest.sii"));
	std::cout << "Ready to get entries." << std::endl;
	std::list<SCSSentries*>* entryList = getEntryListFromFile(*(argv+1), loadToMemory |inflateStream| identFile |decryptSII|getPathList| dropContentAfterDone);
	auto filecount = getResolvedFileCount(entryList);
	do
	{
		filecount = getResolvedFileCount(entryList);
		std::list<std::string>* pathList = entriesToList(*entryList);
		for (auto itr : *pathList)
		{
			auto itr2 = defmap->find(getHash(itr));
			if (itr2 == defmap->end())
				defmap->insert(std::pair<uint64_t, std::string>(getHash(itr), itr));
			if (itr.size() > 3)
			{
				if (itr.substr(itr.size() - 3).compare("pmd") == 0)
					defmap->insert(std::pair<uint64_t, std::string>(getHash(itr.substr(0, itr.size() - 3) + "pmg"), itr.substr(0, itr.size() - 3) + "pmg"));
				if (itr.substr(itr.size() - 3).compare("dds") == 0)
				{
					defmap->insert(std::pair<uint64_t, std::string>(getHash(itr.substr(0, itr.size() - 3) + "tobj"), itr.substr(0, itr.size() - 3) + "tobj"));
					defmap->insert(std::pair<uint64_t, std::string>(getHash(itr.substr(0, itr.size() - 3) + "mat"), itr.substr(0, itr.size() - 3) + "mat"));
				}
				if (itr.substr(itr.size() - 3).compare("mat") == 0)
				{
					defmap->insert(std::pair<uint64_t, std::string>(getHash(itr.substr(0, itr.size() - 3) + "tobj"), itr.substr(0, itr.size() - 3) + "tobj"));
					defmap->insert(std::pair<uint64_t, std::string>(getHash(itr.substr(0, itr.size() - 3) + "dds"), itr.substr(0, itr.size() - 3) + "dds"));
				}
			}
			if (itr.size() > 4)
			{
				if (itr.substr(itr.size() - 4).compare("bank") == 0)
					defmap->insert(std::pair<uint64_t, std::string>(getHash(itr.substr(0, itr.size() - 3) + "bank.guid"), itr.substr(0, itr.size() - 3) + "bank.guid"));
				if (itr.substr(itr.size() - 4).compare("tobj") == 0)
				{
					defmap->insert(std::pair<uint64_t, std::string>(getHash(itr.substr(0, itr.size() - 3) + "dds"), itr.substr(0, itr.size() - 3) + "dds"));
					defmap->insert(std::pair<uint64_t, std::string>(getHash(itr.substr(0, itr.size() - 3) + "mat"), itr.substr(0, itr.size() - 3) + "mat"));
				}
			}
		}
		for (auto itr : *entryList)
		{
			if (!itr->haveFileName())
				itr->searchName(defmap);
			if (itr->haveFileName())
			{
				itr->toAbsPath();
				if (itr->getFileName().size() > 3)
					if (itr->getFileName().substr(itr->getFileName().size() - 3).compare("sui") == 0)
					{
						if (!itr->havecontent())
						{
							std::ifstream ti(*(argv + 1), std::ios::in|std::ios::binary);
							itr->getContent(ti, itr->getFilePos());
							itr->inflateContent();
							itr->setList(extractTextToList(*itr->getContent()));
							itr->dropContent();
						}
						else
							itr->setList(extractTextToList(*itr->getContent()));
					}
				if (itr->getFileName().size() > 8)
					if (itr->getFileName().substr(itr->getFileName().size() - 8).compare("soundref") == 0)
					{
						if (!itr->havecontent())
						{
							std::ifstream ti(*(argv + 1), std::ios::in|std::ios::binary);
							itr->getContent(ti, itr->getFilePos());
							itr->inflateContent();
							itr->setList(extractSoundrefToList(*itr->getContent()));
							itr->dropContent();
						}
						else
							itr->setList(extractTextToList(*itr->getContent()));
					}
			}
		}
	} while (filecount < getResolvedFileCount(entryList));
	std::list<std::string> nameList;
	for (auto itr : *entryList)
	{
		if (itr->haveFileName())
			nameList.push_back(itr->getFileName());
	}
	nameList.sort();
	nameList.unique();
	listToFile(&nameList,"C:\\Users\\hank2\\Desktop\\outlist.txt");
	mapToFile(defmap, "C:\\Users\\hank2\\Desktop\\01.csv");
	std::cout << filecount << " of " << entryList->size() << " resolved." << std::endl;
	return 0;
}