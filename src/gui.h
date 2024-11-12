#pragma once
#include <cstdint>
#include <string>
#include "Shared.h"
#include "nexus/Nexus.h"
#include "mumble/Mumble.h"
#include "imgui/imgui.h"

void DrawBar(float frac, int count, uint64_t totalDamage, const ImVec4& color, const std::string& eliteSpec, bool showDamage, HINSTANCE hSelf);
void RenderSimpleRatioBar(
    const std::vector<float>& counts,
    const std::vector<ImVec4>& colors,
    const ImVec2& size,
    const std::vector<const char*>& texts,
    ImTextureID statIcon);
void RenderTeamData(int teamIndex, const TeamStats& teamData, HINSTANCE hSelf);
void ratioBarSetup(HINSTANCE hSelf);
void RenderSpecializationBars(const TeamStats& teamData, int teamIndex, HINSTANCE hSelf);

