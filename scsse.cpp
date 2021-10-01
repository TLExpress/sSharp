#include <iostream>
#include <string>
#include "scshash.hpp"

int main(int argc, char** argv)
{
	std::string testString;
	while (true)
	{
		std::cin >> testString;
		std::cout << testString << " ¡÷ " << std::hex << getHash(testString) << std::endl;
	}
	return 0;
}