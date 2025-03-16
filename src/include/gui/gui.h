#pragma once
#include <cstdint>
#include <string>
<<<<<<< Updated upstream
<<<<<<< Updated upstream
<<<<<<< Updated upstream
<<<<<<< Updated upstream
<<<<<<< Updated upstream:src/gui.h
#include "Shared.h"
=======
#include "shared/Shared.h"
#include "settings/Settings.h"
>>>>>>> Stashed changes:src/include/gui/gui.h
=======
#include "shared/Shared.h"
#include "settings/Settings.h"
>>>>>>> Stashed changes
=======
#include "shared/Shared.h"
#include "settings/Settings.h"
>>>>>>> Stashed changes
=======
#include "shared/Shared.h"
#include "settings/Settings.h"
>>>>>>> Stashed changes
=======
#include "shared/Shared.h"
#include "settings/Settings.h"
>>>>>>> Stashed changes
#include "nexus/Nexus.h"
#include "mumble/Mumble.h"
#include "imgui/imgui.h"

<<<<<<< Updated upstream
void DrawBar(float frac, int count, uint64_t totalDamage, const ImVec4& color, const std::string& eliteSpec, bool showDamage, HINSTANCE hSelf);
=======
// Helper rendering functions
void DrawBar(
    float frac,
    int count,
    const std::string& barRep,
    const SpecStats& stats,
    const MainWindowSettings* settings,
    const ImVec4& primaryColor,
    const ImVec4& secondaryColor,
    const std::string& eliteSpec,
    HINSTANCE hSelf
);

>>>>>>> Stashed changes
void RenderSimpleRatioBar(
    const std::vector<float>& counts,
    const std::vector<ImVec4>& colors,
    const ImVec2& size,
    const std::vector<const char*>& texts,
<<<<<<< Updated upstream
    ImTextureID statIcon);
void RenderTeamData(int teamIndex, const TeamStats& teamData, HINSTANCE hSelf);
void ratioBarSetup(HINSTANCE hSelf);
void RenderSpecializationBars(const TeamStats& teamData, int teamIndex, HINSTANCE hSelf);
void DrawAggregateStatsWindow(HINSTANCE hSelf);
void RenderMainWindow(HINSTANCE hSelf);
=======
    ImTextureID statIcon,
    const WidgetWindowSettings* settings
);

// Core window rendering functions
void RenderMainWindow(MainWindowSettings* settings, HINSTANCE hSelf);
void RenderMainWindowSettingsPopup(MainWindowSettings* settings);

void RenderSpecializationWindow(MainWindowSettings* settings, HINSTANCE hSelf);
void RenderSpecializationSettingsPopup(MainWindowSettings* settings);
void RenderSpecializationBars(const TeamStats& teamData, const MainWindowSettings* settings, HINSTANCE hSelf);

void RenderWidgetWindow(WidgetWindowSettings* settings, HINSTANCE hSelf);
void RenderWidgetSettingsPopup(WidgetWindowSettings* settings);

void DrawAggregateStatsWindow(HINSTANCE hSelf);
void RenderAggregateSettingsPopup(AggregateWindowSettings* settings);

// Support functions
void RenderTeamData(const TeamStats& teamData, const MainWindowSettings* settings, HINSTANCE hSelf);
void UpdateAggregateStats(const ParsedData& data);

// Main orchestrator
void RenderAllWindows(HINSTANCE hSelf);
>>>>>>> Stashed changes
