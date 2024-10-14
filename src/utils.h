#pragma once

#include <string>
#include <vector>
#include <Windows.h>

// Function declarations
void initMaps();
void monitorDirectory();
void waitForFile(const std::string& filePath);
std::vector<char> extractZipFile(const std::string& filePath);
std::string formatDamage(float damage);
