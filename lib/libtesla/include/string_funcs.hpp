/********************************************************************************
 * File: string_funcs.hpp
 * Author: ppkantorski
 * Description:
 *   This header file contains function declarations and utility functions for string
 *   manipulation. These functions are used in the Ultrahand Overlay project to
 *   perform operations like trimming whitespaces, removing quotes, replacing
 *   multiple slashes, and more.
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
#include <charconv>   // std::from_chars
#include <system_error> // std::errc
#include <sys/stat.h> // struct stat

// Template must stay in header
template <typename T> bool isNumeric(const std::string& str, T* result) {
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), *result);
    return ec == std::errc() && ptr == str.data() + str.size();
};

std::string trim(const std::string& str);
std::string removeWhiteSpaces(const std::string& str);
std::string removeQuotes(const std::string& str);
std::string replaceMultipleSlashes(const std::string& input);
std::string removeLeadingSlash(const std::string& pathPattern);
std::string removeEndingSlash(const std::string& pathPattern);
std::string preprocessPath(const std::string& path);
std::string dropExtension(const std::string& filename);
bool startsWith(const std::string& str, const std::string& prefix);
bool isDirectory(const std::string& path);
bool isFileOrDirectory(const std::string& path);
std::vector<std::string> stringToList(const std::string& str);
