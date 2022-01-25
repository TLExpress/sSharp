#pragma once

#ifndef SCSSHARP_HPP
#define SCSSHARP_HPP

#ifdef SCSSHARP
#define SCSSHARP_DLL __declspec(dllexport)
#else
#define SCSSHARP_DLL __declspec(dllimport)
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <string>
#include <list>
#include <map>
#include "scsFileAccess.hpp"

#endif