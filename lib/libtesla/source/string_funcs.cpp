/********************************************************************************
 * File: string_funcs.cpp
 * Author: ppkantorski
 * Description: Implementation of string utility functions.
 ********************************************************************************/

#include "string_funcs.hpp"
#include <cctype>    // std::isspace, std::isxdigit
#include <sstream>   // std::istringstream (used by stringToList)

/**
 * @brief Trims leading and trailing whitespaces from a string.
 */
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r\f\v");
    size_t last  = str.find_last_not_of(" \t\n\r\f\v");
    if (first == std::string::npos || last == std::string::npos)
        return "";
    return str.substr(first, last - first + 1);
}

/**
 * @brief Removes all white spaces from a string.
 */
std::string removeWhiteSpaces(const std::string& str) {
    std::string result;
    for (char c : str) {
        if (!std::isspace(static_cast<unsigned char>(c)))
            result.push_back(c);
    }
    return result;
}

/**
 * @brief Removes quotes from the beginning and end of a string.
 */
std::string removeQuotes(const std::string& str) {
    std::size_t firstQuote = str.find_first_of("'\"");
    std::size_t lastQuote  = str.find_last_of("'\"");
    if (firstQuote != std::string::npos && lastQuote != std::string::npos && firstQuote < lastQuote)
        return str.substr(firstQuote + 1, lastQuote - firstQuote - 1);
    return str;
}

/**
 * @brief Replaces multiple consecutive slashes with a single slash.
 */
std::string replaceMultipleSlashes(const std::string& input) {
    std::string output;
    bool previousSlash = false;
    for (char c : input) {
        if (c == '/') {
            if (!previousSlash)
                output.push_back(c);
            previousSlash = true;
        } else {
            output.push_back(c);
            previousSlash = false;
        }
    }
    return output;
}

/**
 * @brief Removes the leading slash from a path string if present.
 */
std::string removeLeadingSlash(const std::string& pathPattern) {
    if (!pathPattern.empty() && pathPattern[0] == '/')
        return pathPattern.substr(1);
    return pathPattern;
}

/**
 * @brief Removes the trailing slash from a path string if present.
 */
std::string removeEndingSlash(const std::string& pathPattern) {
    if (!pathPattern.empty() && pathPattern.back() == '/')
        return pathPattern.substr(0, pathPattern.length() - 1);
    return pathPattern;
}

/**
 * @brief Preprocesses a path by normalising slashes and adding "sdmc:" prefix.
 */
std::string preprocessPath(const std::string& path) {
    std::string formattedPath = replaceMultipleSlashes(removeQuotes(path));
    if (formattedPath.compare(0, 5, "sdmc:") != 0)
        return std::string("sdmc:") + formattedPath;
    return formattedPath;
}

/**
 * @brief Drops the file extension from a filename.
 */
std::string dropExtension(const std::string& filename) {
    size_t lastDotPos = filename.find_last_of(".");
    if (lastDotPos != std::string::npos)
        return filename.substr(0, lastDotPos);
    return filename;
}

/**
 * @brief Checks if a string starts with a given prefix.
 */
bool startsWith(const std::string& str, const std::string& prefix) {
    return str.compare(0, prefix.length(), prefix) == 0;
}

/**
 * @brief Checks if a path points to a directory.
 */
bool isDirectory(const std::string& path) {
    struct stat pathStat;
    if (stat(path.c_str(), &pathStat) == 0)
        return S_ISDIR(pathStat.st_mode);
    return false;
}

/**
 * @brief Checks if a path points to a file or directory.
 */
bool isFileOrDirectory(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

/**
 * @brief Splits a string of the form "(a,b,c)" or "[a,b,c]" into a vector.
 */
std::vector<std::string> stringToList(const std::string& str) {
    std::string values, token;
    std::vector<std::string> result;

    if ((str.front() == '(' && str.back() == ')') ||
        (str.front() == '[' && str.back() == ']')) {
        values = str.substr(1, str.size() - 2);
        std::istringstream ss(values);
        while (std::getline(ss, token, ',')) {
            token = token.substr(token.find_first_not_of(" "),
                                 token.find_last_not_of(" ") + 1);
            result.push_back(token);
        }
    }
    return result;
}
