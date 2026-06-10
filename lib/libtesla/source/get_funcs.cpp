/********************************************************************************
 * File: get_funcs.cpp
 * Author: ppkantorski
 * Description: Implementation of file system utility functions.
 ********************************************************************************/

#include "get_funcs.hpp"
#include <cstdio>    // FILE*, fopen, fclose, fread
#include <algorithm> // std::remove
#include <sys/stat.h>
#include <dirent.h>
#include <fnmatch.h>

/**
 * @brief Reads the contents of a file and returns it as a string.
 */
std::string getFileContents(const std::string& filePath) {
    std::string content;
    FILE* file = fopen(filePath.c_str(), "rb");
    if (file) {
        struct stat fileInfo;
        if (stat(filePath.c_str(), &fileInfo) == 0 && fileInfo.st_size > 0) {
            content.resize(fileInfo.st_size);
            fread(&content[0], 1, fileInfo.st_size, file);
        }
        fclose(file);

        // Normalize line endings to '\n'
        content.erase(std::remove(content.begin(), content.end(), '\r'), content.end());
    }
    return content;
}

/**
 * @brief Concatenates the provided directory and file names to form a destination path.
 */
std::string getDestinationPath(const std::string& destinationDir, const std::string& fileName) {
    return destinationDir + "/" + fileName;
}

/**
 * @brief Extracts the value part from a string line containing a key-value pair.
 */
std::string getValueFromLine(const std::string& line) {
    std::size_t equalsPos = line.find('=');
    if (equalsPos != std::string::npos) {
        std::string value = line.substr(equalsPos + 1);
        return trim(value);
    }
    return "";
}

/**
 * @brief Extracts the name from a file path, including handling directories.
 */
std::string getNameFromPath(const std::string& path) {
    size_t lastSlash = path.find_last_of('/');
    if (lastSlash != std::string::npos) {
        std::string name = path.substr(lastSlash + 1);
        if (name.empty()) {
            std::string strippedPath = path.substr(0, lastSlash);
            lastSlash = strippedPath.find_last_of('/');
            if (lastSlash != std::string::npos)
                name = strippedPath.substr(lastSlash + 1);
        }
        return name;
    }
    return path;
}

/**
 * @brief Extracts the file name from a URL.
 */
std::string getFileNameFromURL(const std::string& url) {
    size_t lastSlash = url.find_last_of('/');
    if (lastSlash != std::string::npos)
        return url.substr(lastSlash + 1);
    return "";
}

/**
 * @brief Extracts the name of the parent directory from a given file path.
 */
std::string getParentDirNameFromPath(const std::string& path) {
    std::size_t lastSlashPos = removeEndingSlash(path).rfind('/');
    if (lastSlashPos != std::string::npos && lastSlashPos != 0) {
        std::size_t secondLastSlashPos = path.rfind('/', lastSlashPos - 1);
        if (secondLastSlashPos != std::string::npos) {
            std::string subPath = path.substr(secondLastSlashPos + 1, lastSlashPos - secondLastSlashPos - 1);
            if (subPath.find_first_of(" \t\n\r\f\v") != std::string::npos)
                return "\"" + subPath + "\"";
            return subPath;
        }
    }
    return "";
}

/**
 * @brief Extracts the parent directory path from a given file path.
 */
std::string getParentDirFromPath(const std::string& path) {
    size_t lastSlash = path.find_last_of('/');
    if (lastSlash != std::string::npos)
        return path.substr(0, lastSlash + 1);
    return path;
}

/**
 * @brief Gets a list of subdirectories in a directory.
 */
std::vector<std::string> getSubdirectories(const std::string& directoryPath) {
    std::vector<std::string> subdirectories;
    DIR* dir = opendir(directoryPath.c_str());
    if (dir != nullptr) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string entryName = entry->d_name;
            if (entryName != "." && entryName != "..") {
                struct stat entryStat;
                std::string fullPath = directoryPath + "/" + entryName;
                if (stat(fullPath.c_str(), &entryStat) == 0 && S_ISDIR(entryStat.st_mode))
                    subdirectories.push_back(entryName);
            }
        }
        closedir(dir);
    }
    return subdirectories;
}

/**
 * @brief Recursively retrieves a list of files from a directory.
 */
std::vector<std::string> getFilesListFromDirectory(const std::string& directoryPath) {
    std::vector<std::string> fileList;
    DIR* dir = opendir(directoryPath.c_str());
    if (dir != nullptr) {
        dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string entryName = entry->d_name;
            std::string entryPath = directoryPath;
            if (entryPath.back() != '/')
                entryPath += '/';
            entryPath += entryName;

            if (entryName != "." && entryName != "..") {
                if (isDirectory(entryPath)) {
                    std::vector<std::string> subDirFiles = getFilesListFromDirectory(entryPath);
                    fileList.insert(fileList.end(), subDirFiles.begin(), subDirFiles.end());
                } else {
                    fileList.push_back(entryPath);
                }
            }
        }
        closedir(dir);
    }
    return fileList;
}

/**
 * @brief Gets a list of files and folders based on a wildcard pattern.
 */
std::vector<std::string> getFilesListByWildcard(const std::string& pathPattern) {
    std::string dirPath;
    std::string wildcard;

    std::size_t wildcardPos = pathPattern.find('*');
    if (wildcardPos != std::string::npos) {
        std::size_t slashPos = pathPattern.rfind('/', wildcardPos);
        if (slashPos != std::string::npos) {
            dirPath  = pathPattern.substr(0, slashPos + 1);
            wildcard = pathPattern.substr(slashPos + 1);
        } else {
            dirPath  = "";
            wildcard = pathPattern;
        }
    } else {
        dirPath = pathPattern + "/";
    }

    std::vector<std::string> fileList;
    bool isFolderWildcard = !wildcard.empty() && wildcard.back() == '/';
    if (isFolderWildcard)
        wildcard = wildcard.substr(0, wildcard.size() - 1);

    DIR* dir = opendir(dirPath.c_str());
    if (dir != nullptr) {
        dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string entryName = entry->d_name;
            std::string entryPath = dirPath + entryName;
            bool isEntryDirectory = isDirectory(entryPath);

            if (isFolderWildcard && isEntryDirectory &&
                fnmatch(wildcard.c_str(), entryName.c_str(), FNM_NOESCAPE) == 0) {
                if (entryName != "." && entryName != "..")
                    fileList.push_back(entryPath + "/");
            } else if (!isFolderWildcard && !isEntryDirectory) {
                std::size_t wPos = wildcard.find('*');
                if (wPos != std::string::npos) {
                    std::string prefix = wildcard.substr(0, wPos);
                    if (entryName.find(prefix) == 0) {
                        std::string suffix = wildcard.substr(wPos + 1);
                        if (entryName.size() >= suffix.size() &&
                            entryName.compare(entryName.size() - suffix.size(), suffix.size(), suffix) == 0)
                            fileList.push_back(entryPath);
                    }
                } else if (fnmatch(wildcard.c_str(), entryName.c_str(), FNM_NOESCAPE) == 0) {
                    fileList.push_back(entryPath);
                }
            }
        }
        closedir(dir);
    }
    return fileList;
}

/**
 * @brief Gets a list of files and folders based on a wildcard pattern (multi-wildcard).
 */
std::vector<std::string> getFilesListByWildcards(const std::string& pathPattern) {
    std::vector<std::string> fileList;
    std::size_t wildcardPos = pathPattern.find('*');

    if (wildcardPos != std::string::npos &&
        pathPattern.find('*', wildcardPos + 1) != std::string::npos) {
        std::string dirPath;
        std::string wildcard;

        std::size_t slashPos = pathPattern.rfind('/', wildcardPos);
        if (slashPos != std::string::npos) {
            dirPath  = pathPattern.substr(0, slashPos + 1);
            wildcard = pathPattern.substr(slashPos + 1, wildcardPos - slashPos - 1);
        } else {
            dirPath  = "";
            wildcard = pathPattern.substr(0, wildcardPos);
        }

        std::vector<std::string> subDirs = getFilesListByWildcard(dirPath + wildcard + "*/");
        for (const std::string& subDir : subDirs) {
            std::string subPattern = subDir + removeLeadingSlash(pathPattern.substr(wildcardPos + 1));
            std::vector<std::string> subFileList = getFilesListByWildcards(subPattern);
            fileList.insert(fileList.end(), subFileList.begin(), subFileList.end());
        }
    } else {
        fileList = getFilesListByWildcard(pathPattern);
    }
    return fileList;
}
