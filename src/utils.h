#pragma once

#include <string>
#include <vector>
#include <Windows.h>
#include "imgui/imgui.h"


class Texture;

// Function declarations
ImVec4 GetTeamColor(const std::string& teamName);

void initMaps();
void waitForFile(const std::string& filePath);

Texture** getTextureInfo(const std::string& eliteSpec, int* outResourceId);
std::vector<char> extractZipFile(const std::string& filePath);
std::string formatDamage(double damage);
bool isRunningUnderWine();
