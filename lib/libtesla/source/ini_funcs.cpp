/********************************************************************************
 * File: ini_funcs.cpp
 * Author: ppkantorski
 * Description: Implementation of INI file utility functions.
 ********************************************************************************/

#include "ini_funcs.hpp"
#include <cstring>   // strlen, strncmp
#include <algorithm> // std::remove_if
#include <cctype>    // ::isspace

/**
 * @brief Splits a string into a vector of substrings using a specified delimiter.
 */
std::vector<std::string> split(const std::string& str, char delim) {
    std::vector<std::string> out;
    std::size_t current, previous = 0;
    current = str.find(delim);
    while (current != std::string::npos) {
        out.push_back(str.substr(previous, current - previous));
        previous = current + 1;
        current  = str.find(delim, previous);
    }
    out.push_back(str.substr(previous, current - previous));
    return out;
}

/**
 * @brief Parses an INI-formatted string into a map of sections and key-value pairs.
 */
[[maybe_unused]] std::unordered_map<std::string, std::unordered_map<std::string, std::string>>
parseIni(const std::string& str) {
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> iniData;
    auto lines = split(str, '\n');
    std::string lastHeader;

    for (auto& line : lines) {
        std::string trimmedLine = trim(line);
        if (trimmedLine.empty()) continue;

        if (trimmedLine.front() == '[' && trimmedLine.back() == ']') {
            lastHeader = trimmedLine.substr(1, trimmedLine.size() - 2);
            iniData[lastHeader];
        } else {
            size_t equalPos = trimmedLine.find('=');
            if (equalPos != std::string::npos) {
                std::string key   = trim(trimmedLine.substr(0, equalPos));
                std::string value = trim(trimmedLine.substr(equalPos + 1));
                if (!lastHeader.empty() && !key.empty())
                    iniData[lastHeader].emplace(key, value);
            }
        }
    }
    return iniData;
}

/**
 * @brief Parses an INI file and returns its content as a map of sections and key-value pairs.
 */
std::map<std::string, std::map<std::string, std::string>>
getParsedDataFromIniFile(const std::string& configIniPath) {
    std::map<std::string, std::map<std::string, std::string>> parsedData;
    std::string currentSection;

    FILE* configFileIn = fopen(configIniPath.c_str(), "rb");
    if (!configFileIn)
        return parsedData;

    fseek(configFileIn, 0, SEEK_END);
    long fileSize = ftell(configFileIn);
    rewind(configFileIn);

    char* fileData = new char[fileSize + 1];
    fread(fileData, sizeof(char), fileSize, configFileIn);
    fileData[fileSize] = '\0';
    fclose(configFileIn);

    char* pos = fileData;
    while (*pos) {
        if (*pos == '\r') { ++pos; continue; }

        char* lineStart = pos;
        while (*pos && *pos != '\n') ++pos;
        char* lineEnd = pos;
        if (*pos == '\n') ++pos;

        while (lineEnd > lineStart && *(lineEnd - 1) == '\r') --lineEnd;
        while (lineStart < lineEnd && (*lineStart == ' ' || *lineStart == '\t')) ++lineStart;
        while (lineEnd > lineStart && (*(lineEnd - 1) == ' ' || *(lineEnd - 1) == '\t')) --lineEnd;

        if (lineStart == lineEnd) continue;

        if (*lineStart == '[' && *(lineEnd - 1) == ']') {
            currentSection.assign(lineStart + 1, lineEnd - 1);
        } else {
            char* delim = lineStart;
            while (delim < lineEnd && *delim != '=') ++delim;
            if (delim < lineEnd) {
                char* keyEnd   = delim;
                while (keyEnd > lineStart && (*(keyEnd - 1) == ' ' || *(keyEnd - 1) == '\t')) --keyEnd;

                char* valStart = delim + 1;
                while (valStart < lineEnd && (*valStart == ' ' || *valStart == '\t')) ++valStart;

                parsedData[currentSection].emplace(
                    std::string(lineStart, keyEnd),
                    std::string(valStart, lineEnd)
                );
            }
        }
    }

    delete[] fileData;
    return parsedData;
}

/**
 * @brief Parses sections from an INI file and returns them as a list of strings.
 */
std::vector<std::string> parseSectionsFromIni(const std::string& filePath) {
    std::vector<std::string> sections;
    FILE* file = fopen(filePath.c_str(), "r");
    if (!file)
        return sections;

    constexpr size_t BufferSize = 131072;
    char line[BufferSize];
    while (fgets(line, sizeof(line), file)) {
        std::string trimmedLine = trim(std::string(line));
        if (!trimmedLine.empty() && trimmedLine[0] == '[' && trimmedLine.back() == ']')
            sections.push_back(trimmedLine.substr(1, trimmedLine.size() - 2));
    }
    fclose(file);
    return sections;
}

/**
 * @brief Parses the value of a specific key from an INI section (by file path).
 */
std::string parseValueFromIniSection(const std::string& filePath,
                                     const std::string& sectionName,
                                     const std::string& keyName) {
    std::string value;
    FILE* file = fopen(filePath.c_str(), "r");
    if (!file)
        return value;

    std::string currentSection;
    constexpr size_t BufferSize = 131072;
    char line[BufferSize];

    while (fgets(line, sizeof(line), file)) {
        std::string trimmedLine = trim(std::string(line));
        if (!trimmedLine.empty()) {
            if (trimmedLine[0] == '[' && trimmedLine.back() == ']') {
                currentSection = trimmedLine.substr(1, trimmedLine.size() - 2);
            } else if (currentSection == sectionName) {
                size_t delimiterPos = trimmedLine.find('=');
                if (delimiterPos != std::string::npos) {
                    std::string currentKey = trim(trimmedLine.substr(0, delimiterPos));
                    if (currentKey == keyName) {
                        value = trim(trimmedLine.substr(delimiterPos + 1));
                        break;
                    }
                }
            }
        }
    }
    fclose(file);
    return value;
}

/**
 * @brief Parses the value of a specific key from an INI section (by open FILE handle).
 */
std::string parseValueFromIniSectionF(FILE*& file,
                                      const std::string& sectionName,
                                      const std::string& keyName) {
    std::string value;
    if (!file)
        return value;

    std::string currentSection;
    constexpr size_t BufferSize = 131072;
    char line[BufferSize];

    while (fgets(line, sizeof(line), file)) {
        std::string trimmedLine = trim(std::string(line));
        if (!trimmedLine.empty()) {
            if (trimmedLine[0] == '[' && trimmedLine.back() == ']') {
                currentSection = trimmedLine.substr(1, trimmedLine.size() - 2);
            } else if (currentSection == sectionName) {
                size_t delimiterPos = trimmedLine.find('=');
                if (delimiterPos != std::string::npos) {
                    std::string currentKey = trim(trimmedLine.substr(0, delimiterPos));
                    if (currentKey == keyName) {
                        value = trim(trimmedLine.substr(delimiterPos + 1));
                        break;
                    }
                }
            }
        }
    }
    return value;
}

/**
 * @brief Cleans INI file formatting by removing empty lines and normalising sections.
 */
void cleanIniFormatting(const std::string& filePath) {
    FILE* inputFile = fopen(filePath.c_str(), "r");
    if (!inputFile) return;

    std::string tempPath = filePath + ".tmp";
    FILE* outputFile = fopen(tempPath.c_str(), "w");
    if (!outputFile) {
        fclose(inputFile);
        return;
    }

    bool isNewSection = false;
    char line[4096];
    while (fgets(line, sizeof(line), inputFile)) {
        std::string trimmedLine = trim(std::string(line));
        if (!trimmedLine.empty()) {
            if (trimmedLine[0] == '[' && trimmedLine[trimmedLine.length() - 1] == ']') {
                if (isNewSection)
                    fprintf(outputFile, "\n");
                isNewSection = true;
            }
            fprintf(outputFile, "%s\n", trimmedLine.c_str());
        }
    }
    fclose(inputFile);
    fclose(outputFile);

    remove(filePath.c_str());
    rename(tempPath.c_str(), filePath.c_str());
}

/**
 * @brief Modifies or creates an INI file by adding or updating key-value pairs.
 */
void setIniFile(const std::string& fileToEdit,
                const std::string& desiredSection,
                const std::string& desiredKey,
                const std::string& desiredValue,
                const std::string& desiredNewKey) {
    FILE* configFile = fopen(fileToEdit.c_str(), "r");
    if (!configFile) {
        configFile = fopen(fileToEdit.c_str(), "w");
        if (!configFile) return;
        fprintf(configFile, "[%s]\n%s = %s\n",
                desiredSection.c_str(), desiredKey.c_str(), desiredValue.c_str());
        fclose(configFile);
        return;
    }

    std::string updatedContent;
    std::string currentSection;
    std::string formattedDesiredValue = trim(desiredValue);
    char line[131072];

    bool sectionFound = false;
    bool keyFound     = false;
    bool addNewLine   = false;

    while (fgets(line, sizeof(line), configFile)) {
        std::string trimmedLine = trim(line);

        if (!trimmedLine.empty() && trimmedLine[0] == '[' && trimmedLine.back() == ']') {
            currentSection = removeQuotes(trimmedLine.substr(1, trimmedLine.length() - 2));
            if (sectionFound && !keyFound && desiredNewKey.empty()) {
                if (!updatedContent.empty() && updatedContent.substr(updatedContent.length() - 2) == "\n\n") {
                    updatedContent = updatedContent.substr(0, updatedContent.length() - 1);
                    addNewLine = true;
                }
                updatedContent += desiredKey + " = " + formattedDesiredValue + "\n";
                if (addNewLine) { updatedContent += "\n"; addNewLine = false; }
                keyFound = true;
            }
        }

        if (sectionFound && !keyFound && desiredNewKey.empty() &&
            trim(currentSection) != trim(desiredSection)) {
            if (!updatedContent.empty() && updatedContent.substr(updatedContent.length() - 2) == "\n\n") {
                updatedContent = updatedContent.substr(0, updatedContent.length() - 1);
                addNewLine = true;
            }
            updatedContent += desiredKey + " = " + formattedDesiredValue + "\n";
            if (addNewLine) { updatedContent += "\n"; addNewLine = false; }
            keyFound = true;
        }

        if (trim(currentSection) == trim(desiredSection)) {
            sectionFound = true;
            std::string::size_type delimiterPos = trimmedLine.find('=');
            if (delimiterPos != std::string::npos) {
                std::string lineKey = trim(trimmedLine.substr(0, delimiterPos));
                if (lineKey == desiredKey) {
                    keyFound = true;
                    std::string originalValue = getValueFromLine(trimmedLine);
                    if (!updatedContent.empty() && updatedContent.substr(updatedContent.length() - 2) == "\n\n") {
                        updatedContent = updatedContent.substr(0, updatedContent.length() - 1);
                        addNewLine = true;
                    }
                    if (!desiredNewKey.empty())
                        updatedContent += desiredNewKey + " = " + originalValue + "\n";
                    else
                        updatedContent += desiredKey + " = " + formattedDesiredValue + "\n";
                    if (addNewLine) { updatedContent += "\n"; addNewLine = false; }
                    continue;
                }
            }
        }

        updatedContent += line;
    }

    if (sectionFound && !keyFound && desiredNewKey.empty()) {
        if (!updatedContent.empty() && updatedContent.substr(updatedContent.length() - 2) == "\n\n") {
            updatedContent = updatedContent.substr(0, updatedContent.length() - 1);
            addNewLine = true;
        }
        updatedContent += desiredKey + " = " + formattedDesiredValue + "\n";
        if (addNewLine) { updatedContent += "\n"; addNewLine = false; }
    }

    if (!sectionFound && !keyFound && desiredNewKey.empty())
        updatedContent += "\n[" + desiredSection + "]\n" + desiredKey + " = " + formattedDesiredValue + "\n";

    fclose(configFile);

    configFile = fopen(fileToEdit.c_str(), "w");
    if (!configFile) return;
    fprintf(configFile, "%s", updatedContent.c_str());
    fclose(configFile);
}

/**
 * @brief Sets the value of a key in an INI file within the specified section.
 */
void setIniFileValue(const std::string& fileToEdit,
                     const std::string& desiredSection,
                     const std::string& desiredKey,
                     const std::string& desiredValue) {
    setIniFile(fileToEdit, desiredSection, desiredKey, desiredValue, "");
}

/**
 * @brief Renames a key in an INI file within the specified section.
 */
void setIniFileKey(const std::string& fileToEdit,
                   const std::string& desiredSection,
                   const std::string& desiredKey,
                   const std::string& desiredNewKey) {
    setIniFile(fileToEdit, desiredSection, desiredKey, "", desiredNewKey);
}

/**
 * @brief Adds a new section to an INI file.
 */
void addIniSection(const char* filePath, const char* sectionName) {
    if (!isFileOrDirectory(filePath)) return;

    FILE* inputFile = fopen(filePath, "r");
    if (!inputFile) return;

    FILE* tempFile = fopen("temp.ini", "w");
    if (!tempFile) { fclose(inputFile); return; }

    constexpr size_t BufferSize = 131072;
    char line[BufferSize];
    bool sectionExists = false;

    while (fgets(line, sizeof(line), inputFile)) {
        if (line[0] == '[' && strncmp(&line[1], sectionName, strlen(sectionName)) == 0) {
            sectionExists = true;
            break;
        }
        fputs(line, tempFile);
    }

    if (!sectionExists)
        fprintf(tempFile, "[%s]\n", sectionName);

    while (fgets(line, sizeof(line), inputFile))
        fputs(line, tempFile);

    fclose(inputFile);
    fclose(tempFile);

    remove(filePath);
    rename("temp.ini", filePath);
}

/**
 * @brief Renames a section in an INI file.
 */
void renameIniSection(const std::string& filePath,
                      const std::string& currentSectionName,
                      const std::string& newSectionName) {
    FILE* configFile = fopen(filePath.c_str(), "r");
    if (!configFile) return;

    std::string tempPath = filePath + ".tmp";
    FILE* tempFile = fopen(tempPath.c_str(), "w");
    if (!tempFile) { fclose(configFile); return; }

    bool renaming = false;
    constexpr size_t BufferSize = 131072;
    char line[BufferSize];

    while (fgets(line, sizeof(line), configFile)) {
        std::string currentLine(trim(std::string(line)));

        if (currentLine.length() > 2 && currentLine.front() == '[' && currentLine.back() == ']') {
            std::string sectionName = currentLine.substr(1, currentLine.size() - 2);
            if (sectionName == currentSectionName) {
                fprintf(tempFile, "[%s]\n", newSectionName.c_str());
                renaming = true;
            } else {
                fprintf(tempFile, "%s", currentLine.c_str());
                renaming = false;
            }
        } else if (renaming) {
            fprintf(tempFile, "[%s]\n", newSectionName.c_str());
            renaming = false;
        } else {
            fprintf(tempFile, "%s", currentLine.c_str());
        }
    }

    fclose(configFile);
    fclose(tempFile);

    if (remove(filePath.c_str()) != 0) return;
    rename(tempPath.c_str(), filePath.c_str());
}

/**
 * @brief Removes a section (and all its key-value pairs) from an INI file.
 */
void removeIniSection(const std::string& filePath, const std::string& sectionName) {
    FILE* configFile = fopen(filePath.c_str(), "r");
    if (!configFile) return;

    std::string tempPath = filePath + ".tmp";
    FILE* tempFile = fopen(tempPath.c_str(), "w");
    if (!tempFile) { fclose(configFile); return; }

    bool removing = false;
    constexpr size_t BufferSize = 131072;
    char line[BufferSize];

    while (fgets(line, sizeof(line), configFile)) {
        std::string currentLine(trim(std::string(line)));

        if (currentLine.length() > 2 && currentLine.front() == '[' && currentLine.back() == ']') {
            std::string section = currentLine.substr(1, currentLine.size() - 2);
            if (section == sectionName) {
                removing = true;
            } else {
                fprintf(tempFile, "%s\n", currentLine.c_str());
                removing = false;
            }
        } else if (!removing) {
            fprintf(tempFile, "%s\n", currentLine.c_str());
        }
    }

    fclose(configFile);
    fclose(tempFile);

    if (remove(filePath.c_str()) != 0) return;
    rename(tempPath.c_str(), filePath.c_str());
}
