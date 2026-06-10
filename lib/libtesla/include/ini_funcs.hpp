/********************************************************************************
 * File: ini_funcs.hpp
 * Author: ppkantorski
 * Description:
 *   This header file provides function declarations for working with INI files
 *   in C++, including reading, parsing, and editing INI files.
 *
 *   For the latest updates and contributions, visit the project's GitHub repository.
 *   (GitHub Repository: https://github.com/ppkantorski/Ultrahand-Overlay)
 *
 *   Note: Please be aware that this notice cannot be altered or removed. It is a part
 *   of the project's documentation and must remain intact.
 *
 *  Copyright (c) 2023 ppkantorski
 *  All rights reserved.
 ********************************************************************************/

#pragma once
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <get_funcs.hpp>

// Internal helper – not exposed outside ini_funcs.cpp
std::vector<std::string> split(const std::string& str, char delim = ' ');

[[maybe_unused]] std::unordered_map<std::string, std::unordered_map<std::string, std::string>>
    parseIni(const std::string& str);

std::map<std::string, std::map<std::string, std::string>>
    getParsedDataFromIniFile(const std::string& configIniPath);

std::vector<std::string> parseSectionsFromIni(const std::string& filePath);

std::string parseValueFromIniSection(const std::string& filePath,
                                     const std::string& sectionName,
                                     const std::string& keyName);

std::string parseValueFromIniSectionF(FILE*& file,
                                      const std::string& sectionName,
                                      const std::string& keyName);

void cleanIniFormatting(const std::string& filePath);

void setIniFile(const std::string& fileToEdit,
                const std::string& desiredSection,
                const std::string& desiredKey,
                const std::string& desiredValue,
                const std::string& desiredNewKey);

void setIniFileValue(const std::string& fileToEdit,
                     const std::string& desiredSection,
                     const std::string& desiredKey,
                     const std::string& desiredValue);

void setIniFileKey(const std::string& fileToEdit,
                   const std::string& desiredSection,
                   const std::string& desiredKey,
                   const std::string& desiredNewKey);

void addIniSection(const char* filePath, const char* sectionName);

void renameIniSection(const std::string& filePath,
                      const std::string& currentSectionName,
                      const std::string& newSectionName);

void removeIniSection(const std::string& filePath, const std::string& sectionName);
