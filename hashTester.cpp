#include <iostream>
#include <string>
#include "scshash.hpp"
#include "3nK.hpp"

int main(int argc, char** argv)
{
	
	//while (true)
	{
		std::string testString(*(argv+1));
		//std::cin >> testString;
		//std::string result = testString + "\b\b\b\b_3nK.sii";

		S3nKTranscoder* transcoder = new S3nKTranscoder();
		transcoder->transcodeFileInMemory(testString, testString);
	}
	return 0;
}