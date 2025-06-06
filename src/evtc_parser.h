#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <Windows.h>
#include <mutex>
#include <atomic>
#include <filesystem>


// Function declarations
void monitorDirectory(size_t numLogsToParse, size_t pollIntervalMilliseconds);
std::vector<char> extractZipFile(const std::string& filePath);
void processNewEVTCFile(const std::string& filePath);
void processEVTCFile(const std::filesystem::path& filePath);

// Extern declarations for global variables

extern int currentLogIndex;
extern std::mutex parsedLogsMutex;
extern std::mutex processedFilesMutex;
extern std::atomic<bool> initialParsingComplete;