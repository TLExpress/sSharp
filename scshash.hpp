#pragma once
#include <iostream>
#include <string>
#include <cstdint>

uint64_t getHash(char* str, uint32_t len);
uint64_t getHash(std::string str);

typedef std::pair<uint64_t, uint64_t> uint128_t;
typedef std::pair<uint128_t, uint128_t> uint256_t;