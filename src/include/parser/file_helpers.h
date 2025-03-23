#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <unordered_set>
#include "shared/Shared.h"

// Declare these variables as extern since they're defined elsewhere
extern std::unordered_set<std::wstring> processedFiles;
extern std::filesystem::file_time_type maxProcessedTime;

// File operations
std::vector<char> extractZipFile(const std::string& filePath);
void waitForFile(const std::string& filePath);
std::wstring getCanonicalPath(const std::filesystem::path& path);
std::filesystem::path getArcPath();
bool isValidEVTCFile(const std::filesystem::path& dirPath, const std::filesystem::path& filePath);

// Directory monitoring
void monitorDirectory(size_t numLogsToParse, size_t pollIntervalMilliseconds);
void directoryMonitor(const std::filesystem::path& dirPath, size_t numLogsToParse);
void scanForNewFiles(const std::filesystem::path& dirPath, std::unordered_set<std::wstring>& processedFiles);
void parseInitialLogs(std::unordered_set<std::wstring>& processedFiles, size_t numLogsToParse);
bool isRunningUnderWine();

// Declaration of the function in evtc_parser.cpp that you'll call
void processEVTCFile(const std::string& filePath);
ParsedData parseEVTCFile(const std::string& filePath);