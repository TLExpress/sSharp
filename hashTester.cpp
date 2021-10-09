#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib> 
#include "scshash.hpp"
#include "scsFileAnalyzer.hpp"
#include "SII_Decrypt.hpp"
#include "zlib.h"

int main(int argc, char** argv)
{
	/*size_t size, b, bsize;
	std::ifstream istream(*(argv + 1), std::ios::in | std::ios::binary);
	istream.seekg(0, std::ios::end);
	size = istream.tellg();
	istream.seekg(0, std::ios::beg);
	char* ibuff = new char[size], * obuff = new char[b=bsize = compressBound(size)];
	istream.read(ibuff, size);
	istream.close();
	compress3((Bytef*)obuff, (uLongf*)&bsize, (Bytef*)ibuff, size,15);
	delete[] ibuff;
	std::ofstream ostream(*(argv + 1), std::ios::out | std::ios::binary);
	ostream.write(obuff, bsize);
	ostream.close();
	delete[] obuff;
	std::cout << "compressed" << std::endl;
	system("pause");
	istream.open(*(argv + 1), std::ios::in | std::ios::binary);
	ibuff = new char[b];
	istream.read(ibuff, bsize);
	istream.close();
	obuff = new char[size];
	uncompress3((Bytef*)obuff, (uLongf*)&b, (Bytef*)ibuff, size,15);
	delete[] ibuff;
	ostream.open(*(argv + 1), std::ios::out | std::ios::binary);
	ostream.write(obuff, size);
	ostream.close();
	delete[] obuff;
	std::cout << "uncompressed" << std::endl;
	system("pause");*/

	/*
		std::string a;
		std::cin >> a;
		if (a._Equal("0"))break;
		std::cout << std::hex << getHashFromString(a) << std::endl;
	}*/
	std::string str;
	std::cin >> str;
	std::list<std::string>* list = extractFileToList(str, scsFileAccessMethod::inMemory);
	for (auto const& it : *list)
		std::cout << it << std::endl;
	return 0;
}