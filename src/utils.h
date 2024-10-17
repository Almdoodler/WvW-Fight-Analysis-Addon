#pragma once

#include <string>
#include <vector>
#include <Windows.h>

// Function declarations
void initMaps();
void waitForFile(const std::string& filePath);
std::vector<char> extractZipFile(const std::string& filePath);
std::string formatDamage(double damage);
