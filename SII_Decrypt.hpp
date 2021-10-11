#pragma once

#ifndef SII_DECRYPT_HPP
#define SII_DECRYPT_HPP
#include <cstdint>

#ifdef SII_DECRYPT
#define SII_DECRYPT_DLL __declspec(dllexport)
#else
#define SII_DECRYPT_DLL __declspec(dllimport)
#endif

typedef uint32_t bool32_t;

extern "C" SII_DECRYPT_DLL uint32_t	__stdcall APIVersion();
extern "C" SII_DECRYPT_DLL int32_t	__stdcall GetMemoryFormat(char* Mem,size_t Size);
extern "C" SII_DECRYPT_DLL int32_t	__stdcall GetFileFormat(char* FileName);
extern "C" SII_DECRYPT_DLL bool32_t	__stdcall IsEncryptedMemory(char* Mem, size_t Size);
extern "C" SII_DECRYPT_DLL bool32_t	__stdcall IsEncryptedFile(char* FileName);
extern "C" SII_DECRYPT_DLL bool32_t	__stdcall IsEncodedMemory(char* Mem, size_t Size);
extern "C" SII_DECRYPT_DLL bool32_t	__stdcall IsEncodedFile(char* FileName);
extern "C" SII_DECRYPT_DLL bool32_t	__stdcall Is3nKEncodedMemory(char* Mem, size_t Size);
extern "C" SII_DECRYPT_DLL bool32_t	__stdcall Is3nKEncodedFile(char* FileName);
extern "C" SII_DECRYPT_DLL int32_t	__stdcall DecryptMemory(char* Input, size_t InSize, char* output, size_t* OutSize);
//extern "C" SII_DECRYPT_DLL int32_t	__stdcall DecryptFile(char* InputFile, char* OutputFile);
extern "C" SII_DECRYPT_DLL int32_t	__stdcall DecryptFileInMemory(char* InputFile, char* OutputFile);
extern "C" SII_DECRYPT_DLL int32_t	__stdcall DecodeMemoryHelper(char* Input, size_t InSize, char* output, size_t * OutSize, char** Helper);
extern "C" SII_DECRYPT_DLL int32_t	__stdcall DecodeMemory(char* Input, size_t InSize, char* output, size_t * OutSize);
extern "C" SII_DECRYPT_DLL int32_t	__stdcall DecodeFile(char* InputFile, char* OutputFile);
extern "C" SII_DECRYPT_DLL int32_t	__stdcall DecodeFileInMemory(char* InputFile, char* OutputFile);
extern "C" SII_DECRYPT_DLL int32_t	__stdcall DecryptAndDecodeMemoryHelper(char* Input, size_t InSize, char* output, size_t * OutSize, char** Helper);
extern "C" SII_DECRYPT_DLL int32_t	__stdcall DecryptAndDecodeMemory(char* Input, size_t InSize, char* output, size_t * OutSize);
extern "C" SII_DECRYPT_DLL int32_t	__stdcall DecryptAndDecodeFile(char* InputFile, char* OutputFile);
extern "C" SII_DECRYPT_DLL int32_t	__stdcall DecryptAndDecodeFileInMemory(char* InputFile, char* OutputFile);
extern "C" SII_DECRYPT_DLL int32_t	__stdcall FreeHelper(char** Helper);

#endif