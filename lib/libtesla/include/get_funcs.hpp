/********************************************************************************
 * File: get_funcs.hpp
 * Author: ppkantorski
 * Description:
 *   This header file contains function declarations for retrieving information
 *   and data from various sources, including the file system. It includes
 *   functions for reading file contents, obtaining paths, and listing files.
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
#include <string>
#include <vector>
#include <string_funcs.hpp>

std::string getFileContents(const std::string& filePath);
std::string getDestinationPath(const std::string& destinationDir, const std::string& fileName);
std::string getValueFromLine(const std::string& line);
std::string getNameFromPath(const std::string& path);
std::string getFileNameFromURL(const std::string& url);
std::string getParentDirNameFromPath(const std::string& path);
std::string getParentDirFromPath(const std::string& path);

std::vector<std::string> getSubdirectories(const std::string& directoryPath);
std::vector<std::string> getFilesListFromDirectory(const std::string& directoryPath);
std::vector<std::string> getFilesListByWildcard(const std::string& pathPattern);
std::vector<std::string> getFilesListByWildcards(const std::string& pathPattern);
