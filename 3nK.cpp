#include "3nK.hpp"

/*==============================================================================
	TSII_3nK_Transcoder - implementation
==============================================================================*/

/*------------------------------------------------------------------------------
	TSII_3nK_Transcoder - private methods
------------------------------------------------------------------------------*/

char S3nKTranscoder::getKey(uint8_t index)
{
	return S3nKKeyTable[index];
}

/*------------------------------------------------------------------------------
	TSII_3nK_Transcoder - protected methods
------------------------------------------------------------------------------*/

void S3nKTranscoder::doProgress(double progress)
{
	if (fOnProgressEvent != nullptr)
		fOnProgressEvent(this, progress);
	if (fOnProgressCallback != nullptr)
		fOnProgressCallback(this, progress);
}

//------------------------------------------------------------------------------     

void S3nKTranscoder::transcodeBuffer(char* buff, size_t size, int64_t seed)
{
	size_t i;
	if (size > 0)
		for (i = 0; i < size; i++)
			*(buff + i) = (*(buff + i))^S3nKKeyTable[(uint8_t)(seed+i)];
}

//------------------------------------------------------------------------------     

void S3nKTranscoder::processFile(const std::string inFileName, const std::string outFileName, S3nKProcRoutine routine)
{
	std::ifstream input(inFileName,std::ios::in|std::ios::binary);
	std::ofstream output(outFileName,std::ios::out|std::ios::binary);
	if (input && output)
	{
		(this->*routine)(input, output, false);
		input.close();
		output.close();
	}
}

//------------------------------------------------------------------------------     

void S3nKTranscoder::processFileInMemory(const std::string inFileName, const std::string outFileName, S3nKProcRoutine routine)
{
	std::ifstream inputFile(inFileName, std::ios::in | std::ios::binary);
	std::stringstream input(std::ios::in | std::ios::out | std::ios::binary);
	input << inputFile.rdbuf();
	inputFile.close();
	std::stringstream output(std::ios::in | std::ios::out | std::ios::binary);
	(this->*routine)(input, output, false);
	input.clear();
	output.seekg(0, std::ios::beg);
	std::ofstream outputFile(outFileName, std::ios::out | std::ios::binary);
	output.seekg(0, std::ios::beg);
	outputFile << output.rdbuf();
	outputFile.close();
	output.clear();
}

/*------------------------------------------------------------------------------
	TSII_3nK_Transcoder - public methods
------------------------------------------------------------------------------*/

S3nKTranscoder::S3nKTranscoder()
{
	fseed = 0;
	fOnProgressEvent = nullptr;
	fOnProgressCallback = nullptr;
}

//------------------------------------------------------------------------------     

bool S3nKTranscoder::is3nKStream(std::istream& stream)
{
	S3nKHeader header;
	size_t pos = (size_t)stream.tellg();
	stream.seekg(0, std::ios::end);
	size_t size = (size_t)stream.tellg();
	stream.seekg(pos, std::ios::beg);
	if (size - pos >= S3nKMSize)
	{
		stream.read((char*)&header, sizeofHeader);
		stream.seekg(pos, std::ios::beg);
		return header.sign == S3nKSign;
	}
	/*else*/ return false;
}

//------------------------------------------------------------------------------     

bool S3nKTranscoder::is3nKFile(std::string FileName)
{
	std::ifstream input("Filename", std::ios::in, std::ios::binary);
	if (input)
		return is3nKStream(input);
	/*else*/ return false;
}

//------------------------------------------------------------------------------     

void S3nKTranscoder::encodeStream(std::istream& input, std::ostream& output, bool invariantOutput)
{
	S3nKHeader header;
	char* buff = new char[S3nK_BufferSize];
	int64_t actualReg;
	int64_t progressStart;
	size_t restOfInput, readSize, inputSize;
	
	srand((size_t)time(NULL));

	doProgress(0.0);
	header.sign = S3nKSign;
	header.unk = 0;
	header.seed = (uint8_t)rand()%255+1;
	fseed = header.seed;
	actualReg = fseed;
	output.write((char*)&header, sizeofHeader);
	input.seekg(0, std::ios::end);
	if ((inputSize = restOfInput = (size_t)input.tellg()) > 0)
	{
		input.seekg(0, std::ios::beg);
		progressStart = input.tellg();
		do
		{
			readSize = restOfInput < S3nK_BufferSize ? restOfInput : S3nK_BufferSize;
			restOfInput -= readSize;
			input.read(buff, readSize);
			transcodeBuffer(buff, readSize, actualReg);
			output.write(buff, readSize);
			actualReg += readSize;
			doProgress((double)(input.tellg() - progressStart) / (double)(inputSize - progressStart));
		} while (restOfInput >= S3nK_BufferSize);
	}
}

//------------------------------------------------------------------------------     

void S3nKTranscoder::decodeStream(std::istream& input, std::ostream& output, bool invariantOutput)
{
	S3nKHeader header;
	char *buff = new char[S3nK_BufferSize];
	int64_t actualReg;
	int64_t progressStart;
	size_t restOfInput, readSize, inputSize;

	if (is3nKStream(input))
	{
		doProgress(0.0);
		input.read((char*)&header, sizeofHeader);
		actualReg = header.seed;
		fseed = header.seed;
		input.seekg(0, std::ios::end);
		if ((inputSize = restOfInput = (size_t)input.tellg()) > sizeofHeader)
		{
			input.seekg(sizeofHeader, std::ios::beg);
			progressStart = input.tellg();
			do
			{
				readSize = restOfInput < S3nK_BufferSize ? restOfInput : S3nK_BufferSize;
				restOfInput -= readSize;
				input.read(buff, readSize);
				transcodeBuffer(buff, readSize, actualReg);
				output.write(buff, readSize);
				actualReg += readSize;
				doProgress((double)(input.tellg() - progressStart) / (double)(inputSize - progressStart));
			} while (restOfInput >= S3nK_BufferSize);
		}
	}
	else
		std::cerr << "Input stream is not a valid 3nK stream." << std::endl;
}

//------------------------------------------------------------------------------     

void S3nKTranscoder::transcodeStream(std::istream& input, std::ostream& output, bool invariantOutput)
{
	if (is3nKStream(input))
		decodeStream(input, output, invariantOutput);
	else
		encodeStream(input, output, invariantOutput);
}

//------------------------------------------------------------------------------     

void S3nKTranscoder::decodeFile(std::string inFileName, std::string outFileName)
{
	processFile(inFileName, outFileName, &S3nKTranscoder::transcodeStream);
}

//------------------------------------------------------------------------------     

void S3nKTranscoder::encodeFile(std::string inFileName, std::string outFileName)
{
	processFile(inFileName, outFileName, &S3nKTranscoder::transcodeStream);
}

//------------------------------------------------------------------------------     

void S3nKTranscoder::transcodeFile(std::string inFileName, std::string outFileName)
{
	processFile(inFileName, outFileName, &S3nKTranscoder::transcodeStream);
}

//------------------------------------------------------------------------------     

void S3nKTranscoder::decodeFileInMemory(std::string inFileName, std::string outFileName)
{
	processFileInMemory(inFileName, outFileName, &S3nKTranscoder::transcodeStream);
}

//------------------------------------------------------------------------------     

void S3nKTranscoder::encodeFileInMemory(std::string inFileName, std::string outFileName)
{
	processFileInMemory(inFileName, outFileName, &S3nKTranscoder::transcodeStream);
}

//------------------------------------------------------------------------------     

void S3nKTranscoder::transcodeFileInMemory(std::string inFileName, std::string outFileName)
{
	processFileInMemory(inFileName, outFileName, &S3nKTranscoder::transcodeStream);
}