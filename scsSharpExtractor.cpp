#include "scsSharpExtractor.hpp"

int main(int argc, char** argv)
{
	if (argc <= 1)
	{
		std::cout << "No files." << std::endl;
		return 0;
	}
	std::map<uint64_t, std::string>* defmap = folderToMap("E:\\SCS\\141");
	defmap->merge(*LogToMap("C:\\Users\\hank2\\Documents\\Euro Truck Simulator 2\\game.log.txt"));
	defmap->insert(std::pair<uint64_t, std::string>(0x9ae16a3b2f90404f, ""));
	defmap->insert(std::pair<uint64_t, std::string>(0xb97fff7ce7377c95, "manifest.sii"));
	std::cout << "Ready to get entries." << std::endl;
	size_t* filecount = new size_t[(size_t)argc - 1];
	std::list<SCSSentries*>** entryList = new std::list<SCSSentries*>*[(size_t)argc - 1];
	bool FurtherIndex;
	for (int c = 1; c < argc; c++)
		entryList[c-1] = scssToEntries(*(argv + c),
			loadToMemory | inflateStream | identFile | decryptSII | getPathList | dropContentAfterDone);
	do
	{
		FurtherIndex = false;
		for (int c = 1; c < argc; c++)
		{
			filecount[c - 1] = getResolvedFileCount(entryList[c - 1]);
			std::list<std::string>* pathList = entriesToList(*entryList[c - 1]);
			for (auto itr : *pathList)
			{
				auto itr2 = defmap->find(getHash(itr));
				if (itr2 == defmap->end())
					defmap->insert(std::pair<uint64_t, std::string>(getHash(itr), itr));
			}
		}
		for (int c = 1; c < argc; c++)
		{
			for (auto itr : *entryList[c - 1])
			{
				if (!itr->haveFileName())
				{
					itr->searchName(defmap);
					if (itr->haveFileName())
					{
						if (itr->getFileName().size() > 3)
							if (itr->getFileName().substr(itr->getFileName().size() - 3).compare("sui") == 0)
							{
								if (!itr->havecontent())
								{
									std::ifstream ti(*(argv + 1), std::ios::in | std::ios::binary);
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
									std::ifstream ti(*(argv + 1), std::ios::in | std::ios::binary);
									itr->getContent(ti, itr->getFilePos());
									itr->inflateContent();
									itr->setList(extractSoundrefToList(*itr->getContent()));
									itr->dropContent();
								}
								else
									itr->setList(extractTextToList(*itr->getContent()));
							}
						itr->toAbsPath();
					}
				}
			}
		}
		for (int c = 1; c < argc; c++)
		{
			if (filecount[c - 1] < getResolvedFileCount(entryList[c - 1]))
			{
				FurtherIndex = true;
				break;
			}
		}
	} while (FurtherIndex);
	std::list<std::string>* nameList = new std::list<std::string>[(size_t)argc-1];
	for (int c = 1; c < argc; c++)
	{
		for (auto itr : *entryList[c-1])
		{
			if (itr->haveFileName())
				nameList[c - 1].push_back(itr->getFileName());
		}
		nameList[c - 1].sort();
		nameList[c - 1].unique();
		listToFile(&nameList[c - 1], ((std::string)*(argv+c)) + ".txt");
		entryToFile(entryList[c - 1], ((std::string) * (argv + c)).substr(0, ((std::string) * (argv + c)).find_last_of(".")), (std::string) * (argv + c));
		std::cout << filecount[c - 1] << " of " << entryList[c - 1]->size() << " resolved." << std::endl;
	}
	system("pause");
	return 0;
}