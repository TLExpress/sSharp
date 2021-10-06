/****************************************************************************/
/*                                                                          */
/*   Code algorithm Copyright (c) 2002 by SCS Software (except CityHash64). */
/*                                                                          */
/*   See the file LICENSE.TXT included in this distribution, for details    */
/*   about the copyright.                                                   */
/*                                                                          */
/****************************************************************************/

#include "scsHash.hpp"
#include "city.h"

/****************************************************************************/
/*   helper functions                                                       */
/****************************************************************************/

uint64_t hashMULXOR(uint64_t hash1, uint64_t hash2)
{
	uint64_t result;
	result = (hash1 ^ hash2) * 0x9ddfea08eb382d69ULL;
	result = ((result >> 0x2fULL) ^ result ^ hash2) * 0x9ddfea08eb382d69ULL;
	result = ((result >> 0x2fULL) ^ result) * 0x9ddfea08eb382d69ULL;
	return result;
}

void hashADDINV(uint128_t& hh, uint64_t hash1, uint64_t hash2, uint64_t hash3, uint64_t hash4, uint64_t hash5, uint64_t hash6)
{
	uint64_t temp1, temp2, temp3;
	temp1 = hash1 + hash5;
	temp2 = hash4 + temp1 + hash6;
	temp3 = (temp2 << 0x2bULL) | (temp2 >> 0x15ULL);
	temp2 = hash2 + hash3 + temp1;
	hh = uint128_t((temp2 + hash4), ((temp2 >> 0x2c) | (temp2 << 0x14)) + temp1 + temp3 ) ;
}

void hashADDINVBUF(uint256_t hashx4, uint128_t& hh, uint64_t hash1, uint64_t hash2)
{
	hashADDINV(
		hh,
		hashx4.first.first,
		hashx4.first.second,
		hashx4.second.first,
		hashx4.second.second,
		hash1,
		hash2
	);
}

/****************************************************************************/
/*   hash generation                                                        */
/****************************************************************************/

uint64_t getHashDefault()
{
	return 0x9ae16a3b2f90404f;
}

uint64_t getHash1to3(char* str, uint32_t len)
{
	uint32_t temp;
	uint64_t hash;

	if (str != nullptr && len > 0 && len < 4)
	{
		temp = (uint32_t)*str + (uint32_t) * (str + (len >> 1)) * 0x100UL;
		hash = (uint64_t)temp * 0x9ae16a3b2f90404fULL;
		temp = len + (uint32_t) * (str + len - 1) * 4;
		hash = hash ^ (uint64_t)temp * 0xc949d7c7509e6557ULL;
		return ((hash >> 0x2fULL) ^ hash) * 0x9ae16a3b2f90404fULL;
	}
	/*else*/ return 0ULL;
}

uint64_t getHash4to8(char* str, uint32_t len)
{
	if (str != nullptr && len >= 4 && len <= 8)
		return hashMULXOR((uint64_t) * (uint32_t*)str * 8 + len, (uint64_t) * (uint32_t*)(str + len - 4));
	/*else*/ return 0ULL;
}

uint64_t getHash9to16(char* str, uint32_t len)
{
	uint64_t hash1, hash2, hash3;
	uint32_t temp;
	if (str != nullptr && len > 8 && len <= 16)
	{
		hash1 = *(uint64_t*)str;
		hash2 = *(uint64_t*)(str + len - 8UL);
		temp = len & 0x3fUL;
		hash3 = hash2 + (uint64_t)len;
		return hashMULXOR(hash1, (hash3 << (0x40U - temp)) | (hash3 >> temp)) ^ hash2;
	}
	/*else*/ return 0ULL;
}

uint64_t getHash17to32(char* str, uint32_t len)
{
	uint64_t hash1, hash2, hash3, hash4, hash5, hash6;
	if (str != nullptr && len > 16 && len <= 32)
	{
		hash1 = *(uint64_t*)str * 0xb492b66fbe98f273ULL;
		hash2 = *(uint64_t*)(str + 8UL) ^ 0xc949d7c7509e6557;
		hash3 = *(uint64_t*)(str + len - 0x08UL) * 0x9ae16a3b2f90404fULL;
		hash4 = *(uint64_t*)(str + len - 0x10UL) * 0xc3a5c85c97cb3127ULL;
		hash5 = (hash2 >> 0x14U) | (hash2 << 0x2cU);
		hash2 = hash1 - *(uint64_t*)(str + 8UL);
		hash6 = (hash2 << 0x15U) | (hash2 >> 0x2bU);
		return hashMULXOR(((hash3 >> 0x1eU) | (hash3 << 0x22U)) + hash6 + hash4, (uint64_t)len - hash3 + hash5 + hash1);
	}
	/*else*/ return 0ULL;
}

uint64_t getHash33to64(char* str, uint32_t len)
{
	uint64_t hash1, hash2, hash3, hash4, hash5, hash6, hash7, hash8, hash9;
	uint64_t result;
	if (str != nullptr && len > 32UL && len <= 64UL)
	{
		hash1 = *(uint64_t*)(str + 0x18UL);
		hash2 = *(uint64_t*)(str + len - 0x10UL);
		hash3 = (hash2 + (uint64_t)len) * 0xc3a5c85c97cb3127ULL + *(uint64_t*)str;
		result = hash3 + hash1;
		hash4 = (result << 0x0cU) | (result >> 0x34U);
		hash5 = (hash3 << 0x1bU) | (hash3 >> 0x25U);
		hash3 = hash3 + *(uint64_t*)(str + 8UL);
		hash6 = hash3 << 0x39U;
		hash5 = hash5 + ((hash3 >> 0x07U) | hash6);
		result = *(uint64_t*)(str + 0x10UL) + hash3;
		hash6 = result + *(uint64_t*)(str + 0x18UL);
		hash7 = ((result >> 0x1fU) | (result << 0x21U)) + hash5 + hash4;
		hash8 = *(uint64_t*)(str + len - 8UL);
		result = *(uint64_t*)(str + len - 0x20UL) + *(uint64_t*)(str + 0x10UL);
		hash4 = result + hash8;
		hash4 = (hash4 << 0x0cU) | (hash4 >> 0x34U);
		hash5 = (result << 0x1bU) | (result >> 0x25U);
		result = result + *(uint64_t*)(str + len - 18UL);
		hash5 = hash5 + ((result >> 0x07U) | (result << 0x39U));
		result = result + hash2;
		hash9 = (((result >> 0x1fU) | (result << 0x21U)) + hash6 + hash5 + hash4) * 0x9ae16a3b2f90404fULL;
		result = (result + hash8 + hash7) * 0xc3a5c85c97cb3127ULL + hash9;
		result = ((result >> 0x2fU) ^ result) * 0xc3a5c85c97cb3127ULL + hash7;
		result = ((result >> 0x2fU) ^ result) * 0x9ae16a3b2f90404fULL;
		return result;
	}
	/*else*/ return 0ULL;
}

uint64_t getHash65up(char* str, uint32_t len)  //Buggy, using the cityhash instead. :(
{
	uint64_t hash1, hash2, hash3, hash4;
	uint64_t result;
	uint128_t hh1, hh2;
	uint32_t c;
	uint32_t cidx;
	if (str != nullptr && len > 64UL)
	{
		hash1 = *(uint64_t*)(str + len - 0x38UL) + *(uint64_t*)(str + len - 0x10UL);
		hash2 = hashMULXOR(*(uint64_t*)(str + len - 0x30UL) + (uint64_t)len, *(uint64_t*)(str + len - 0x18UL));

		hashADDINVBUF(
			uint256_t(uint128_t(*(uint64_t*)(str + len - 0x40UL), *(uint64_t*)(str + len)),uint128_t(*(uint64_t*)(str + len + 0x40UL),*(uint64_t*)(str + len + 0x80UL))),
			//*(uint256_t*)(str + len - 0x40UL),
			hh1,
			(uint64_t)len,
			hash2
		);

		hashADDINVBUF(
			uint256_t(uint128_t(*(uint64_t*)(str + len - 0x20UL), *(uint64_t*)(str + len + 0x20UL)), uint128_t(*(uint64_t*)(str + len + 0x60UL), *(uint64_t*)(str + len + 0x100UL))),
			//*(uint256_t*)(str + len - 0x20UL),
			hh2,
			hash1 + (uint64_t)len,
			*(uint64_t*)(str + len - 0x28UL)
		);
		hash3 = *(uint64_t*)(str + len - 0x28UL) * 0xb492b66fbe98f273ULL + *(uint64_t*)str;
		c = (len - 1) >> 0x06U;
		cidx = 0x30UL;

		while (c > 0)
		{
			hash4 = *(uint64_t*)(str + cidx - 0x28UL) + hh1.first + hash1 + hash3;
			hash3 = ((hash4 << 0x1bU) | (hash4 >> 0x25U)) * 0xb492b66fbe98f273ULL;
			hash4 = *(uint64_t*)(str + cidx) + hh1.second + hash1;
			hash1 = ((hash4 << 0x16U) | (hash4 >> 0x2aU)) * 0xb492b66fbe98f273ULL + *(uint64_t*)(str + cidx - 0x08UL) + hh1.first;
			hash3 = hash3 ^ hh2.second;
			hash4 = hh2.first + hash2;
			hash2 = ((hash4 << 0x1fU) | (hash4 >> 0x21U)) * 0xb492b66fbe98f273ULL;

			hashADDINV(
				hh1,
				*(uint64_t*)(str + cidx - 0x30UL),
				*(uint64_t*)(str + cidx - 0x28UL),
				*(uint64_t*)(str + cidx - 0x20UL),
				*(uint64_t*)(str + cidx - 0x18UL),
				hh1.second * 0xb492b66fbe98f273ULL,
				hh2.first + hash3
			);

			hashADDINV(
				hh2,
				*(uint64_t*)(str + cidx - 0x10UL),
				*(uint64_t*)(str + cidx - 0x08UL),
				*(uint64_t*)(str + cidx),
				*(uint64_t*)(str + cidx + 0x08UL),
				hh2.second + hash2,
				*(uint64_t*)(str + cidx - 0x20UL) + hash1
			);

			hash4 = hash3;
			hash3 = hash2;
			hash2 = hash4;
			cidx += 0x40UL;
			c--;
		}

		hash4 = hashMULXOR(hh1.first, hh2.first);
		result = ((hash1 >> 0x2fU) ^ hash1) * 0xb492b66fbe98f273ULL + hash4 + hash2;
		result = hashMULXOR(result, hashMULXOR(hh1.second, hh2.second) + hash3);
		return result;
	}
	/*else*/ return 0;
}

/****************************************************************************/
/*   exported functions                                                     */
/****************************************************************************/

uint64_t getHash(char* str, uint32_t len)
{
	if (len > 0)
	{
		if (len < 0x04UL)return getHash1to3(str, len);
		if (len <= 0x08UL)return getHash4to8(str, len);
		if (len <= 0x10UL)return getHash9to16(str, len);
		if (len <= 0x20UL)return getHash17to32(str, len);
		if (len <= 0x40UL)return getHash33to64(str, len);
		/*else*/return CityHash64(str, len);
	}
	/*else*/return getHashDefault();
}

uint64_t getHash(std::string str)
{
	return getHash((char*)str.c_str(), (uint32_t)str.size());
}