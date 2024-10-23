#pragma once
#include <cstdint>
#include <string>
#include "Shared.h"
#include "nexus/Nexus.h"
#include "mumble/Mumble.h"
#include "imgui/imgui.h"

void DrawBar(float frac, int count, uint64_t totalDamage, const ImVec4& color, const std::string& eliteSpec, bool showDamage, HINSTANCE hSelf);
void RenderSimpleRatioBar(int red, int green, int blue,
    const ImVec4& colorRed, const ImVec4& colorGreen, const ImVec4& colorBlue,
    const ImVec2& size);
void RenderTeamData(int teamIndex, const TeamStats& teamData, HINSTANCE hSelf);
void ratioBarSetup();