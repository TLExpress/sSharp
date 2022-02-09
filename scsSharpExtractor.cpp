#include "scsSharpExtractor.hpp"

using namespace scsFileAccess;

int main(int argc, char** argv)
{
	namespace em = EntryMode;
	SCSDictionary defmap = make_shared<_SCSDictionary>();
	stdfs::directory_entry base_folder("E:\\SteamLibrary\\steamapps\\common\\Euro Truck Simulator 2");
	auto list = make_shared<vector<SCSEntryList>>();
	for (auto itr : stdfs::recursive_directory_iterator(base_folder))
		if (itr.path().string().substr(itr.path().string().size() - 4).compare(".scs") == 0 && !itr.is_directory())
			list->push_back(scssToEntries(itr.path().string(), em::inflateStream | em::identFile | em::getPathList | em::foldersOnly));
	auto map_size = defmap->size();
	do
	{
		map_size = defmap->size();
		for (auto itr : *list)
			analyzeEntriesWithMap(itr, defmap);
	} while (map_size < defmap->size());
	auto deflist = convertMapToList(defmap);
	saveListToFile(deflist,"C:\\Users\\hank2\\Desktop\\144_base.txt");
	return 0;
	/*namespace em = EntryMode;
	SCSDictionary defmap = convertListToMap(getListFromFile("C:\\Users\\hank2\\Desktop\\143_base.txt"));
	defmap->merge(*getMapFromLog("C:\\Users\\hank2\\Documents\\Euro Truck Simulator 2\\game.log.txt"));
	auto entryList = zipToEntries("C:\\Users\\hank2\\Desktop\\STARLIGHT.zip", em::inflateStream | em::decryptSII | em::identFile | em::getPathList | em::calculateCrc);
	analyzeEntriesWithMap(entryList, defmap);
	buildFolder(entryList);
	decryptEntryList(entryList, [](auto itr) {return itr->have_file_name; });
	deflateEntryList(entryList, [](auto itr) {return itr->decrypted; },CompressedType::ZIP);
	entryToScss(entryList, "C:\\Users\\hank2\\Desktop\\STARLIGHT3.scs");
	SCSPathList list = make_shared<_SCSPathList>();
	for (auto itr : *entryList)
		if(itr->have_file_name)
			list->push_back(itr->file_name);
	sort(list->begin(), list->end());
	list->erase(unique(list->begin(),list->end()),list->end());
	//saveListToFile(list, "C:\\Users\\hank2\\Desktop\\STARLIGHT.key.txt");
	return 0;*/
}