#include <Windows.h>
#include "resource.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <thread>
#include <mutex>
#include <atomic>
#include "nexus/Nexus.h"
#include "mumble/Mumble.h"
#include "imgui/imgui.h"
#include "Shared.h"
#include "Settings.h"
#include "gui.h"
#include "utils.h"
#include <zip.h>
#include <regex>
#include <chrono>
#include <shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

#include "evtc_parser.h"
#include "utils.h"

// Function prototypes
void AddonLoad(AddonAPI* aApi);
void AddonUnload();
void AddonRender();
void AddonOptions();
void ProcessKeybinds(const char* aIdentifier);

/* globals */
AddonDefinition AddonDef = {};
HMODULE hSelf = nullptr;

std::filesystem::path AddonPath;
std::filesystem::path SettingsPath;





BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH: hSelf = hModule; break;
    case DLL_PROCESS_DETACH: break;
    case DLL_THREAD_ATTACH: break;
    case DLL_THREAD_DETACH: break;
    }
    return TRUE;
}

extern "C" __declspec(dllexport) AddonDefinition * GetAddonDef()
{
    AddonDef.Signature = -996748;
    AddonDef.APIVersion = NEXUS_API_VERSION;
    AddonDef.Name = ADDON_NAME;
    AddonDef.Version.Major = 1;
    AddonDef.Version.Minor = 0;
    AddonDef.Version.Build = 2;
    AddonDef.Version.Revision = 1;
    AddonDef.Author = "Unreal";
    AddonDef.Description = "Simple WvW log analysis tool.";
    AddonDef.Load = AddonLoad;
    AddonDef.Unload = AddonUnload;
    AddonDef.Flags = EAddonFlags_None;
    AddonDef.Provider = EUpdateProvider_GitHub;
    AddonDef.UpdateLink = "https://github.com/jake-greygoose/WvW-Fight-Analysis-Addon";
    return &AddonDef;
}

void AddonLoad(AddonAPI* aApi)
{
    APIDefs = aApi;
    ImGui::SetCurrentContext((ImGuiContext*)APIDefs->ImguiContext);
    ImGui::SetAllocatorFunctions((void* (*)(size_t, void*))APIDefs->ImguiMalloc, (void(*)(void*, void*))APIDefs->ImguiFree);
    NexusLink = (NexusLinkData*)APIDefs->GetResource("DL_NEXUS_LINK");
    MumbleLink = (Mumble::Data*)APIDefs->GetResource("DL_MUMBLE_LINK");

    APIDefs->RegisterRender(ERenderType_OptionsRender, AddonOptions);
    APIDefs->RegisterRender(ERenderType_Render, AddonRender);

    AddonPath = APIDefs->GetAddonDirectory("WvWFightAnalysis");
    SettingsPath = APIDefs->GetAddonDirectory("WvWFightAnalysis/settings.json");
    std::filesystem::create_directory(AddonPath);
    Settings::Load(SettingsPath);

    APIDefs->RegisterKeybindWithString("KB_WINDOW_TOGGLEVISIBLE", ProcessKeybinds, "(null)");
    APIDefs->RegisterKeybindWithString("KB_WIDGET_TOGGLEVISIBLE", ProcessKeybinds, "(null)");

    APIDefs->RegisterKeybindWithString("LOG_INDEX_UP", ProcessKeybinds, "(null)");
    APIDefs->RegisterKeybindWithString("LOG_INDEX_DOWN", ProcessKeybinds, "(null)");

    Downed = APIDefs->GetTextureOrCreateFromResource("DOWNED_ICON", DOWNED, hSelf);
    Death = APIDefs->GetTextureOrCreateFromResource("DEATH_ICON", DEATH, hSelf);
    Squad = APIDefs->GetTextureOrCreateFromResource("SQUAD_ICON", SQUAD, hSelf);
    initMaps();

    directoryMonitorThread = std::thread(monitorDirectory, Settings::logHistorySize);

    APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, "Addon loaded successfully.");
}



void AddonUnload()
{
    stopMonitoring = true;

    // Signal the monitoring thread to stop and wait for it
    if (directoryMonitorThread.joinable())
    {
        directoryMonitorThread.join();
    }

    // Wait for the initial parsing thread to finish
    if (initialParsingThread.joinable())
    {
        initialParsingThread.join();
    }

    APIDefs->DeregisterRender(AddonRender);
    APIDefs->DeregisterRender(AddonOptions);
    APIDefs->DeregisterKeybind("KB_WINDOW_TOGGLEVISIBLE");
    APIDefs->DeregisterKeybind("KB_WIDGET_TOGGLEVISIBLE");
    APIDefs->DeregisterKeybind("LOG_INDEX_UP");
    APIDefs->DeregisterKeybind("LOG_INDEX_DOWN");

    APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, "Addon unloaded successfully.");
}


void ProcessKeybinds(const char* aIdentifier)
{
    std::string str = aIdentifier;

    if (str == "KB_MI_TOGGLEVISIBLE")
    {
        Settings::IsAddonWindowEnabled = !Settings::IsAddonWindowEnabled;
        Settings::Save(SettingsPath);
    }
    else if (str == "KB_WIDGET_TOGGLEVISIBLE")
    {
        Settings::IsAddonWidgetEnabled = !Settings::IsAddonWidgetEnabled;
        Settings::Save(SettingsPath);
    }
    else if (str == "LOG_INDEX_DOWN") 
    {
        if (!parsedLogs.empty()) {
            currentLogIndex = (currentLogIndex - 1 + parsedLogs.size()) % static_cast<int>(parsedLogs.size());
        }
        else {
            APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
                ("Log Index: " + std::to_string(currentLogIndex)).c_str());
        }

    }
    else if (str == "LOG_INDEX_UP")
    {
        if (!parsedLogs.empty()) {
            currentLogIndex = (currentLogIndex + 1) % static_cast<int>(parsedLogs.size());
        }
        else {
            APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
                ("Log Index: " + std::to_string(currentLogIndex)).c_str());
        }
    } 
}


void AddonRender()
{
    std::lock_guard<std::mutex> lock(parsedLogsMutex);

    if (currentLogIndex >= parsedLogs.size())
    {
        currentLogIndex = 0; // Reset to the latest log if out of bounds
    }

    if (!NexusLink || !NexusLink->IsGameplay || !MumbleLink || MumbleLink->Context.IsMapOpen)
    {
        return;
    }
    // If not on a WvW map then hide window
    if (
        MumbleLink->Context.MapType != Mumble::EMapType::WvW_EternalBattlegrounds &&
        MumbleLink->Context.MapType != Mumble::EMapType::WvW_BlueBorderlands &&
        MumbleLink->Context.MapType != Mumble::EMapType::WvW_GreenBorderlands &&
        MumbleLink->Context.MapType != Mumble::EMapType::WvW_RedBorderlands &&
        MumbleLink->Context.MapType != Mumble::EMapType::WvW_ObsidianSanctum &&
        MumbleLink->Context.MapType != Mumble::EMapType::WvW_EdgeOfTheMists &&
        MumbleLink->Context.MapType != Mumble::EMapType::WvW_Lounge
        ) {
        return;
    }

    // Hide if in Combat
    if (MumbleLink->Context.IsInCombat && !Settings::showWindowInCombat) { return; }

    if (Settings::IsAddonWidgetEnabled) {
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

        float barHeight = 20.0f;
        ImVec2 barSize = ImVec2(320.0f, barHeight);
        ImGui::SetNextWindowSize(barSize);

        if (ImGui::Begin("Team Ratio Bar", nullptr, window_flags))
        {
            if (parsedLogs.empty())
            {
                ImGui::Text(initialParsingComplete ? "No logs parsed yet." : "Parsing logs...");
                ImGui::End();
                ImGui::PopStyleVar(4);
                return;
            }

            const auto& currentLogData = parsedLogs[currentLogIndex].data;
            const char* team_names[] = { "Red", "Blue", "Green" };
            ImVec4 team_colors[] = {
                ImGui::ColorConvertU32ToFloat4(IM_COL32(0xff, 0x44, 0x44, 0xff)), // Red
                ImGui::ColorConvertU32ToFloat4(IM_COL32(0x33, 0xb5, 0xe5, 0xff)), // Blue
                ImGui::ColorConvertU32ToFloat4(IM_COL32(0x99, 0xcc, 0x00, 0xff))  // Green
            };

            int teamCounts[3] = { 0, 0, 0 };
            for (int i = 0; i < 3; ++i)
            {
                auto teamIt = currentLogData.teamStats.find(team_names[i]);
                if (teamIt != currentLogData.teamStats.end())
                {
                    teamCounts[i] = teamIt->second.totalPlayers;
                }
            }

            RenderSimpleRatioBar(
                teamCounts[0],        // Red count
                teamCounts[2],        // Green count
                teamCounts[1],        // Blue count
                team_colors[0],       // Red color
                team_colors[2],       // Green color
                team_colors[1],       // Blue color
                ImVec2(barSize.x, barHeight)
            );
        }
        ImGui::End();
        ImGui::PopStyleVar(4);

    }

    if (Settings::IsAddonWindowEnabled)
    {
        ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("WvW Fight Analysis", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse))
        {

            ImGuiStyle& style = ImGui::GetStyle();
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(style.FramePadding.x, 5));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x, 10));
            float sz = ImGui::GetFontSize();
            if (parsedLogs.empty())
            {
                if (initialParsingComplete)
                {
                    ImGui::Text("No logs parsed yet.");
                }
                else
                {
                    ImGui::Text("Parsing logs...");
                }
                ImGui::PopStyleVar(2);
                ImGui::End();
                return;
            }

            std::string fnstr = parsedLogs[currentLogIndex].filename.substr(0, parsedLogs[currentLogIndex].filename.find_last_of('.'));
            uint64_t durationMs = parsedLogs[currentLogIndex].data.combatEndTime - parsedLogs[currentLogIndex].data.combatStartTime;
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::milliseconds(durationMs));
            int minutes = duration.count() / 60;
            int seconds = duration.count() % 60;
            std::string displayName = fnstr + " (" + std::to_string(minutes) + "m" + std::to_string(seconds) + "s)";

            ImGui::Text("%s", displayName.c_str());

            const auto& currentLogData = parsedLogs[currentLogIndex].data;
            const char* team_names[] = { "Red", "Blue", "Green" };
            int teamCounts[3] = { 0, 0, 0 };
            ImVec4 team_colors[] = {
                ImGui::ColorConvertU32ToFloat4(IM_COL32(0xff, 0x44, 0x44, 0xff)), // Red
                ImGui::ColorConvertU32ToFloat4(IM_COL32(0x33, 0xb5, 0xe5, 0xff)), // Blue
                ImGui::ColorConvertU32ToFloat4(IM_COL32(0x99, 0xcc, 0x00, 0xff))  // Green
            };
            
            int teamsWithData = 0;
            bool teamHasData[3] = { false, false, false };
            for (int i = 0; i < 3; ++i) {
                auto teamIt = currentLogData.teamStats.find(team_names[i]);
                if (teamIt != currentLogData.teamStats.end() && teamIt->second.totalPlayers >= Settings::teamPlayerThreshold) {
                    teamsWithData++;
                    teamHasData[i] = true;
                }
            }
            ImGuiTableFlags table_flags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoPadOuterX;
            if (Settings::showScrollBar)
            {
                table_flags |= ImGuiTableFlags_ScrollY;
            }
            else
            {
                table_flags |= ImGuiTableFlags_NoKeepColumnsVisible;
            }
            if (teamsWithData == 0) {
                ImGui::Text("No team data available meeting the player threshold.");
            }
            else {
                if (Settings::useTabbedView) {
                    // Render using tabs
                    if (ImGui::BeginTabBar("TeamTabBar", ImGuiTabBarFlags_None))
                    {
                        for (int i = 0; i < 3; ++i) {
                            if (teamHasData[i]) {
                                // Set the tab label with team name and color
                                ImGui::PushStyleColor(ImGuiCol_Text, team_colors[i]);
                                if (ImGui::BeginTabItem(team_names[i])) {
                                    ImGui::PopStyleColor(); // Pop the color after starting the tab item

                                    // Begin child region with optional scroll bar
                                    ImGuiWindowFlags child_flags = ImGuiWindowFlags_NoMove;
                                    if (!Settings::showScrollBar) {
                                        child_flags |= ImGuiWindowFlags_NoScrollbar;
                                    }
                                    // Note: Do not use ImGuiWindowFlags_AlwaysVerticalScrollbar

                                    // Determine child region size
                                    ImVec2 child_size = ImVec2(0, 0); // Use available space

                                    ImGui::BeginChild(("TeamChild" + std::to_string(i)).c_str(), child_size, false, child_flags);

                                    const char* team_name = team_names[i];
                                    auto teamIt = currentLogData.teamStats.find(team_name);
                                    if (teamIt != currentLogData.teamStats.end())
                                    {
                                        const auto& teamData = teamIt->second;
                                        // Call the rendering function
                                        RenderTeamData(i, teamData);
                                    }

                                    ImGui::EndChild();

                                    ImGui::EndTabItem();
                                }
                                else {
                                    ImGui::PopStyleColor(); // Ensure color is popped even if the tab item isn't active
                                }
                            }
                        }
                        ImGui::EndTabBar();
                    }
                }

                else {
                    ImGuiTableFlags table_flags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoPadOuterX;

                    if (Settings::showScrollBar)
                    {
                        table_flags |= ImGuiTableFlags_ScrollY;
                    }
                    else
                    {
                        table_flags |= ImGuiTableFlags_NoKeepColumnsVisible;
                    }

                    ImVec2 table_size = ImVec2(0.0f, ImGui::GetContentRegionAvail().y);

                    if (ImGui::BeginTable("TeamTable", teamsWithData, table_flags, table_size))
                    {
                        ImGui::TableSetupScrollFreeze(0, 1);

                        for (int i = 0; i < 3; ++i) {
                            if (teamHasData[i]) {
                                ImGui::TableSetupColumn(team_names[i], ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthStretch);
                            }
                        }

                        ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
                        int columnIndex = 0;
                        for (int i = 0; i < 3; ++i) {
                            if (teamHasData[i]) {
                                ImGui::TableSetColumnIndex(columnIndex++);
                                ImGui::PushStyleColor(ImGuiCol_Text, team_colors[i]);
                                ImGui::Text("%s Team", team_names[i]);
                                ImGui::PopStyleColor();
                            }
                        }

                        ImGui::TableNextRow();
                        columnIndex = 0;
                        for (int i = 0; i < 3; ++i) {
                            if (teamHasData[i]) {
                                ImGui::TableSetColumnIndex(columnIndex++);

                                const char* team_name = team_names[i];
                                auto teamIt = currentLogData.teamStats.find(team_name);
                                if (teamIt != currentLogData.teamStats.end())
                                {
                                    const auto& teamData = teamIt->second;
                                    RenderTeamData(i, teamData);
                                }
                            }
                        }

                        ImGui::EndTable();
                    }
                }
            }





            ImGui::PopStyleVar(2);

            // Right-click menu for log history selection
            if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_ChildWindows) && ImGui::IsMouseClicked(1))  // Right mouse button
            {
                ImGui::OpenPopup("Log Selection");
            }

            if (ImGui::BeginPopup("Log Selection"))
            {
                if (ImGui::BeginMenu("History"))
                {
                    for (int i = 0; i < parsedLogs.size(); ++i)
                    {
                        const auto& log = parsedLogs[i];

                        std::string fnstr = log.filename.substr(0, log.filename.find_last_of('.'));
                        uint64_t durationMs = log.data.combatEndTime - log.data.combatStartTime;
                        auto duration = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::milliseconds(durationMs));
                        int minutes = duration.count() / 60;
                        int seconds = duration.count() % 60;
                        std::string displayName = fnstr + " (" + std::to_string(minutes) + "m" + std::to_string(seconds) + "s)";

                        if (ImGui::RadioButton(displayName.c_str(), &currentLogIndex, i))
                        {

                        }
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Display")) {
                    if (ImGui::Checkbox("Show Class Bars", &Settings::showSpecBars))
                    {
                        Settings::Settings[SHOW_SPEC_BARS] = Settings::showSpecBars;
                        Settings::Save(SettingsPath);
                    }
                    if (ImGui::Checkbox("Short Class Names", &Settings::useShortClassNames))
                    {
                        Settings::Settings[USE_SHORT_CLASS_NAMES] = Settings::useShortClassNames;
                        Settings::Save(SettingsPath);
                    }
                    if (ImGui::Checkbox("Show Class Names", &Settings::showClassNames))
                    {
                        Settings::Settings[SHOW_CLASS_NAMES] = Settings::showClassNames;
                        Settings::Save(SettingsPath);
                    }
                    if (ImGui::Checkbox("Show Class Icons", &Settings::showClassIcons))
                    {
                        Settings::Settings[SHOW_CLASS_ICONS] = Settings::showClassIcons;
                        Settings::Save(SettingsPath);
                    }
                    if (ImGui::Checkbox("Show Class Damage", &Settings::showSpecDamage))
                    {
                        Settings::Settings[SHOW_SPEC_DAMAGE] = Settings::showSpecDamage;
                        Settings::Save(SettingsPath);
                    }
                    if (ImGui::Checkbox("Sort Class Damage", &Settings::sortSpecDamage))
                    {
                        Settings::Settings[SORT_SPEC_DAMAGE] = Settings::sortSpecDamage;
                        Settings::Save(SettingsPath);
                    }
                    ImGui::Separator();
                    if (ImGui::Checkbox("Team Count", &Settings::showTeamTotalPlayers))
                    {
                        Settings::Settings[SHOW_TEAM_TOTAL_PLAYERS] = Settings::showTeamTotalPlayers;
                        Settings::Save(SettingsPath);
                    }
                    if (ImGui::Checkbox("Team Deaths", &Settings::showTeamDeaths))
                    {
                        Settings::Settings[SHOW_TEAM_DEATHS] = Settings::showTeamDeaths;
                        Settings::Save(SettingsPath);
                    }
                    if (ImGui::Checkbox("Team Downed", &Settings::showTeamDowned))
                    {
                        Settings::Settings[SHOW_TEAM_DOWNED] = Settings::showTeamDowned;
                        Settings::Save(SettingsPath);
                    }
                    if (ImGui::Checkbox("Team Damage", &Settings::showTeamDamage))
                    {
                        Settings::Settings[SHOW_TEAM_DAMAGE] = Settings::showTeamDamage;
                        Settings::Save(SettingsPath);
                    }
                    if (ImGui::Checkbox("Team Strike Damage", &Settings::showTeamStrikeDamage))
                    {
                        Settings::Settings[SHOW_TEAM_STRIKE] = Settings::showTeamStrikeDamage;
                        Settings::Save(SettingsPath);
                    }
                    if (ImGui::Checkbox("Team Condi Damage", &Settings::showTeamCondiDamage))
                    {
                        Settings::Settings[SHOW_TEAM_CONDI] = Settings::showTeamCondiDamage;
                        Settings::Save(SettingsPath);
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Style")) {
                    if (ImGui::Checkbox("Show Scroll Bar", &Settings::showScrollBar))
                    {
                        Settings::Settings[SHOW_SCROLL_BAR] = Settings::showScrollBar;
                        Settings::Save(SettingsPath);
                    }
                    if (ImGui::Checkbox("Use Tabbed View", &Settings::useTabbedView))
                    {
                        Settings::Settings[USE_TABBED_VIEW] = Settings::useTabbedView;
                        Settings::Save(SettingsPath);
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndPopup();
            }

            ImGui::End();
        }
        else
        {
            ImGui::End();
        }
    }
}


void AddonOptions()
{
    ImGui::Text("WvW Fight Analysis Settings");
    if (ImGui::Checkbox("Window Enabled##WvWFightAnalysis", &Settings::IsAddonWindowEnabled))
    {
        Settings::Settings[IS_ADDON_WINDOW_VISIBLE] = Settings::IsAddonWindowEnabled;
        Settings::Save(SettingsPath);
    }
    if (ImGui::Checkbox("Widget Enabled##WvWFightAnalysis", &Settings::IsAddonWidgetEnabled))
    {
        Settings::Settings[IS_ADDON_WIDGET_VISIBLE] = Settings::IsAddonWidgetEnabled;
        Settings::Save(SettingsPath);
    }
    if (ImGui::Checkbox("Visible In Combat##WvWFightAnalysis", &Settings::showWindowInCombat))
    {
        Settings::Settings[IS_WINDOW_VISIBLE_IN_COMBAT] = Settings::showWindowInCombat;
        Settings::Save(SettingsPath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Untick to hide in combat.");
        ImGui::EndTooltip();
    }
    ImGui::Text("Team Player Threshold: ");
    if (ImGui::InputInt("Team Player Threshold##WvWFightAnalysis", &Settings::teamPlayerThreshold))
    {
        Settings::teamPlayerThreshold = std::clamp(
            Settings::teamPlayerThreshold, 0,100
        );
        Settings::Settings[TEAM_PLAYER_THRESHOLD] = Settings::teamPlayerThreshold;
        Settings::Save(SettingsPath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Set a minimum amount of team players required to render team.");
        ImGui::EndTooltip();
    }
    int tempLogHistorySize = static_cast<int>(Settings::logHistorySize);
    if (ImGui::InputInt("Log History Size##WvWFightAnalysis", &tempLogHistorySize))
    {
        Settings::logHistorySize = static_cast<size_t>(std::clamp(tempLogHistorySize, 1, 20));
        Settings::Settings[LOG_HISTORY_SIZE] = Settings::logHistorySize;
        Settings::Save(SettingsPath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("How many parsed logs to keep.");
        ImGui::EndTooltip();
    }
    if (ImGui::InputText("Custom Log Path##WvWFightAnalysis", Settings::LogDirectoryPathC, sizeof(Settings::LogDirectoryPathC)))
    {
        Settings::LogDirectoryPath = Settings::LogDirectoryPathC;
        Settings::Settings[CUSTOM_LOG_PATH] = Settings::LogDirectoryPath;
        Settings::Save(SettingsPath);
    }

}