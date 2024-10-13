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



Texture** getTextureInfo(const std::string& eliteSpec, int* outResourceId) {
    if (eliteSpec == "Berserker") { *outResourceId = BERSERKER; return &Berserker; }
    else if (eliteSpec == "Bladesworn") { *outResourceId = BLADESWORN; return &Bladesworn; }
    else if (eliteSpec == "Catalyst") { *outResourceId = CATALYST; return &Catalyst; }
    else if (eliteSpec == "Chronomancer") { *outResourceId = CHRONOMANCER; return &Chronomancer; }
    else if (eliteSpec == "Daredevil") { *outResourceId = DAREDEVIL; return &Daredevil; }
    else if (eliteSpec == "Deadeye") { *outResourceId = DEADEYE; return &Deadeye; }
    else if (eliteSpec == "Dragonhunter") { *outResourceId = DRAGONHUNTER; return &Dragonhunter; }
    else if (eliteSpec == "Druid") { *outResourceId = DRUID; return &Druid; }
    else if (eliteSpec == "Core Elementalist") { *outResourceId = ELEMENTALIST; return &Elementalist; }
    else if (eliteSpec == "Core Engineer") { *outResourceId = ENGINEER; return &Engineer; }
    else if (eliteSpec == "Firebrand") { *outResourceId = FIREBRAND; return &Firebrand; }
    else if (eliteSpec == "Core Guardian") { *outResourceId = GUARDIAN; return &Guardian; }
    else if (eliteSpec == "Harbinger") { *outResourceId = HARBINGER; return &Harbinger; }
    else if (eliteSpec == "Herald") { *outResourceId = HERALD; return &Herald; }
    else if (eliteSpec == "Holosmith") { *outResourceId = HOLOSMITH; return &Holosmith; }
    else if (eliteSpec == "Mechanist") { *outResourceId = MECHANIST; return &Mechanist; }
    else if (eliteSpec == "Core Mesmer") { *outResourceId = MESMER; return &Mesmer; }
    else if (eliteSpec == "Mirage") { *outResourceId = MIRAGE; return &Mirage; }
    else if (eliteSpec == "Core Necromancer") { *outResourceId = NECROMANCER; return &Necromancer; }
    else if (eliteSpec == "Core Ranger") { *outResourceId = RANGER; return &Ranger; }
    else if (eliteSpec == "Reaper") { *outResourceId = REAPER; return &Reaper; }
    else if (eliteSpec == "Renegade") { *outResourceId = RENEGADE; return &Renegade; }
    else if (eliteSpec == "Core Revenant") { *outResourceId = REVENANT; return &Revenant; }
    else if (eliteSpec == "Scrapper") { *outResourceId = SCRAPPER; return &Scrapper; }
    else if (eliteSpec == "Scourge") { *outResourceId = SCOURGE; return &Scourge; }
    else if (eliteSpec == "Soulbeast") { *outResourceId = SOULBEAST; return &Soulbeast; }
    else if (eliteSpec == "Specter") { *outResourceId = SPECTER; return &Specter; }
    else if (eliteSpec == "Spellbreaker") { *outResourceId = SPELLBREAKER; return &Spellbreaker; }
    else if (eliteSpec == "Tempest") { *outResourceId = TEMPEST; return &Tempest; }
    else if (eliteSpec == "Core Thief") { *outResourceId = THIEF; return &Thief; }
    else if (eliteSpec == "Untamed") { *outResourceId = UNTAMED; return &Untamed; }
    else if (eliteSpec == "Vindicator") { *outResourceId = VINDICATOR; return &Vindicator; }
    else if (eliteSpec == "Virtuoso") { *outResourceId = VIRTUOSO; return &Virtuoso; }
    else if (eliteSpec == "Core Warrior") { *outResourceId = WARRIOR; return &Warrior; }
    else if (eliteSpec == "Weaver") { *outResourceId = WEAVER; return &Weaver; }
    else if (eliteSpec == "Willbender") { *outResourceId = WILLBENDER; return &Willbender; }
    else { *outResourceId = 0; return nullptr; }
}


void DrawBar(const float frac, const char* text, const ImVec4& color, const std::string& eliteSpec)
{
    ImVec2 cursor_pos = ImGui::GetCursorPos();
    ImVec2 screen_pos = ImGui::GetCursorScreenPos();
    float bar_width = ImGui::GetContentRegionAvail().x * frac;
    float bar_height = ImGui::GetTextLineHeight() + 4; // Add some padding

    // Draw the bar
    ImGui::GetWindowDrawList()->AddRectFilled(
        screen_pos,
        ImVec2(screen_pos.x + bar_width, screen_pos.y + bar_height),
        ImGui::ColorConvertFloat4ToU32(color)
    );

    // Set cursor position for text
    ImGui::SetCursorPos(ImVec2(cursor_pos.x + 5, cursor_pos.y + 2)); // Add some padding

    // Draw count
    int count;
    sscanf(text, "%d", &count);
    ImGui::Text("%d", count);

    // Calculate position for icon
    float count_width = ImGui::GetItemRectSize().x;
    ImGui::SameLine(0, 5); // Add some space between count and icon

    // Draw icon
    if (Settings::showClassIcons)
    {
        float sz = ImGui::GetFontSize();
        int resourceId;
        Texture** texturePtrPtr = getTextureInfo(eliteSpec, &resourceId);

        if (texturePtrPtr && *texturePtrPtr && (*texturePtrPtr)->Resource)
        {
            ImGui::Image((*texturePtrPtr)->Resource, ImVec2(sz, sz));
        }
        else
        {
            if (resourceId != 0 && texturePtrPtr) {
                *texturePtrPtr = APIDefs->GetTextureOrCreateFromResource((eliteSpec + "_ICON").c_str(), resourceId, hSelf);
                if (*texturePtrPtr && (*texturePtrPtr)->Resource)
                {
                    ImGui::Image((*texturePtrPtr)->Resource, ImVec2(sz, sz));
                }
                else
                {
                    ImGui::Text("%c%c", eliteSpec[0], eliteSpec[1]); // Use first two letters as fallback
                }
            }
            else
            {
                ImGui::Text("%c%c", eliteSpec[0], eliteSpec[1]); // Use first two letters as fallback
            }
        }
        ImGui::SameLine(0, 5); // Add some space between icon and name
    }

    // Draw elite spec name
    if (Settings::showClassNames)
    {
        if (Settings::useShortClassNames) 
        {
            std::string shortClassName;
            auto clnIt = eliteSpecShortNames.find(eliteSpec);
            if (clnIt != eliteSpecShortNames.end()) {
                shortClassName = clnIt->second;
                ImGui::Text("%s", shortClassName.c_str());
            }
            else 
            {
                shortClassName = "Unk";
            }
        }
        else 
        {
            ImGui::Text("%s", eliteSpec.c_str());
        }      
    }
    else 
    {
        ImGui::Text(" ", eliteSpec.c_str());
    }

    // Move cursor to next line for the next bar
    ImGui::SetCursorPosY(cursor_pos.y + bar_height + 2); // Add a small gap between bars
}

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
    AddonDef.Version.Build = 1;
    AddonDef.Version.Revision = 5;
    AddonDef.Author = "jake-greygoose";
    AddonDef.Description = "EVTC Parser for WvW logs";
    AddonDef.Load = AddonLoad;
    AddonDef.Unload = AddonUnload;
    AddonDef.Flags = EAddonFlags_None;
    AddonDef.Provider = EUpdateProvider_GitHub;
    AddonDef.UpdateLink = "https://github.com/jake-greygoose/EVTC-Team-Counter";
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

    AddonPath = APIDefs->GetAddonDirectory("MistInsight");
    SettingsPath = APIDefs->GetAddonDirectory("MistInsight/settings.json");
    std::filesystem::create_directory(AddonPath);
    Settings::Load(SettingsPath);

    APIDefs->RegisterKeybindWithString("KB_MI_TOGGLEVISIBLE", ProcessKeybinds, "(null)");

    Downed = APIDefs->GetTextureOrCreateFromResource("DOWNED_ICON", DOWNED, hSelf);
    Death = APIDefs->GetTextureOrCreateFromResource("DEATH_ICON", DEATH, hSelf);
    Squad = APIDefs->GetTextureOrCreateFromResource("SQUAD_ICON", SQUAD, hSelf);
    initMaps();

    // Start the initial parsing in a separate thread
    initialParsingThread = std::thread(parseInitialLogs);

    // Start the directory monitoring thread
    directoryMonitorThread = std::thread(monitorDirectory);

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
    APIDefs->DeregisterKeybind("KB_MI_TOGGLEVISIBLE");
    
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

    if (Settings::IsAddonWindowEnabled)
    {
        if (ImGui::Begin("Mist Insights", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse))
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

            std::regex dateTimeRegex(R"((\d{8}-\d{6})\.zevtc)");
            std::smatch match;
            std::string dateTimeStr;
            if (std::regex_search(parsedLogs[currentLogIndex].filename, match, dateTimeRegex)) {
                dateTimeStr = match[1];
            }
            else {
                dateTimeStr = "Unknown";
            }
            uint64_t durationMs = parsedLogs[currentLogIndex].data.combatEndTime - parsedLogs[currentLogIndex].data.combatStartTime;
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::milliseconds(durationMs));
            int minutes = duration.count() / 60;
            int seconds = duration.count() % 60;
            std::string displayName = dateTimeStr + " (" + std::to_string(minutes) + "m" + std::to_string(seconds) + "s)";

            ImGui::Text("Current Log: %s", displayName.c_str());

            const auto& currentLogData = parsedLogs[currentLogIndex].data;
            const char* team_names[] = { "Red", "Blue", "Green" };
            ImVec4 team_colors[] = {
                ImGui::ColorConvertU32ToFloat4(IM_COL32(0xff, 0x44, 0x44, 0xff)), // Red
                ImGui::ColorConvertU32ToFloat4(IM_COL32(0x33, 0xb5, 0xe5, 0xff)), // Blue
                ImGui::ColorConvertU32ToFloat4(IM_COL32(0x99, 0xcc, 0x00, 0xff))  // Green
            };

            // Count how many teams have data and meet the threshold
            int teamsWithData = 0;
            bool teamHasData[3] = { false, false, false };
            for (int i = 0; i < 3; ++i) {
                auto teamIt = currentLogData.teamStats.find(team_names[i]);
                if (teamIt != currentLogData.teamStats.end() && teamIt->second.totalPlayers >= Settings::teamPlayerThreshold) {
                    teamsWithData++;
                    teamHasData[i] = true;
                }
            }

            if (teamsWithData == 0) {
                ImGui::Text("No team data available meeting the player threshold.");
            }
            else if (ImGui::BeginTable("TeamTable", teamsWithData, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_ScrollY))
            {
                ImGui::TableSetupScrollFreeze(0, 1);

                // team player threshhold
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
                            ImGui::Spacing();
                            if (Settings::showClassIcons)
                            {
                                if (Squad && Squad->Resource)
                                {
                                    ImGui::Image(Squad->Resource, ImVec2(sz, sz));
                                    ImGui::SameLine(0, 5);
                                }
                                else {
                                    Squad = APIDefs->GetTextureOrCreateFromResource("SQUAD_ICON", SQUAD, hSelf);
                                }
                            }
                            if (Settings::showClassNames)
                            {
                                ImGui::Text("Total: %d", teamData.totalPlayers);
                            }
                            else
                            {
                                ImGui::Text("%d", teamData.totalPlayers);
                            }
                            if (Settings::showClassIcons) 
                            {
                                if (Death && Death->Resource)
                                {
                                    ImGui::Image(Death->Resource, ImVec2(sz, sz));
                                    ImGui::SameLine(0, 5);
                                }
                                else 
                                {
                                    Death = APIDefs->GetTextureOrCreateFromResource("DEATH_ICON", DEATH, hSelf);
                                }
                            }
                            if (Settings::showClassNames)
                            {
                                ImGui::Text("Deaths: %d", teamData.totalDeaths);
                            }
                            else
                            {
                                ImGui::Text("%d", teamData.totalDeaths);
                            }
                            if (Settings::showClassIcons)
                            {
                                if (Downed && Downed->Resource)
                                {
                                    ImGui::Image(Downed->Resource, ImVec2(sz, sz));
                                    ImGui::SameLine(0, 5);
                                }
                                else 
                                {
                                    Downed = APIDefs->GetTextureOrCreateFromResource("DOWNED_ICON", DOWNED, hSelf);
                                }
                            }
                            if (Settings::showClassNames)
                            {
                                ImGui::Text("Downed: %d", teamData.totalDowned);
                            }
                            else
                            {
                                ImGui::Text("%d", teamData.totalDowned);
                            }
                            ImGui::Separator();

                            // Sort classes by count in descending order
                            std::vector<std::pair<std::string, int>> sortedClasses(teamData.eliteSpecCounts.begin(), teamData.eliteSpecCounts.end());
                            std::sort(sortedClasses.begin(), sortedClasses.end(),
                                [](const auto& a, const auto& b) { return a.second > b.second; });

                            int maxCount = sortedClasses.empty() ? 0 : sortedClasses[0].second;

                            for (const auto& specPair : sortedClasses)
                            {
                                const std::string& eliteSpec = specPair.first;
                                int count = specPair.second;
                                float frac = (maxCount > 0) ? static_cast<float>(count) / maxCount : 0.0f;

                                char buffer[64];
                                snprintf(buffer, sizeof(buffer), "%d %s", count, eliteSpec.c_str());

                                std::string professionName;
                                auto it = eliteSpecToProfession.find(eliteSpec);
                                if (it != eliteSpecToProfession.end()) {
                                    professionName = it->second;
                                }
                                else {
                                    professionName = "Unknown";
                                }

                                ImVec4 color;
                                auto colorIt = professionColors.find(professionName);
                                if (colorIt != professionColors.end()) {
                                    color = colorIt->second;
                                }
                                else {
                                    color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                                }

                                DrawBar(frac, buffer, color, eliteSpec);
                            }
                        }
                    }
                }

                ImGui::EndTable();
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

                        std::regex dateTimeRegex(R"((\d{8}-\d{6})\.zevtc)");
                        std::smatch match;
                        std::string dateTimeStr;
                        if (std::regex_search(log.filename, match, dateTimeRegex)) {
                            dateTimeStr = match[1];
                        }
                        else {
                            dateTimeStr = "Unknown";
                        }

                        uint64_t durationMs = log.data.combatEndTime - log.data.combatStartTime;
                        auto duration = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::milliseconds(durationMs));
                        int minutes = duration.count() / 60;
                        int seconds = duration.count() % 60;
                        std::string displayName = dateTimeStr + " (" + std::to_string(minutes) + "m" + std::to_string(seconds) + "s)";

                        if (ImGui::RadioButton(displayName.c_str(), &currentLogIndex, i))
                        {
                            // Your existing selection logic here
                        }
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
    ImGui::Text("Mist Insight Settings");
    if (ImGui::Checkbox("Enabled##MistInsight", &Settings::IsAddonWindowEnabled))
    {
        Settings::Settings[IS_ADDON_WINDOW_VISIBLE] = Settings::IsAddonWindowEnabled;
        Settings::Save(SettingsPath);
    }
    if (ImGui::Checkbox("Visible In Combat##MistInsight", &Settings::showWindowInCombat))
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
    if (ImGui::Checkbox("Show Class Names##MistInsight", &Settings::showClassNames))
    {
        Settings::Settings[SHOW_CLASS_NAMES] = Settings::showClassNames;
        Settings::Save(SettingsPath);
    }
    if (ImGui::Checkbox("Use Short Class Names##MistInsight", &Settings::useShortClassNames))
    {
        Settings::Settings[USE_SHORT_CLASS_NAMES] = Settings::useShortClassNames;
        Settings::Save(SettingsPath);
    }
    if (ImGui::Checkbox("Show Class Icons##MistInsight", &Settings::showClassIcons))
    {
        Settings::Settings[SHOW_CLASS_ICONS] = Settings::showClassIcons;
        Settings::Save(SettingsPath);
    }
    ImGui::Text("Team Player Threshold: ");
    if (ImGui::InputInt("Team Player Threshold##MistInsight", &Settings::teamPlayerThreshold))
    {
        Settings::Settings[TEAM_PLAYER_THRESHOLD] = Settings::teamPlayerThreshold;
        Settings::Save(SettingsPath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Set a minimum amount of team players required to render column.");
        ImGui::EndTooltip();
    }
    ImGui::Text("Custom Log Path: ");
    if (ImGui::InputText("Log Path##MistInsight", Settings::LogDirectoryPathC, sizeof(Settings::LogDirectoryPathC)))
    {
        Settings::LogDirectoryPath = Settings::LogDirectoryPathC;
        Settings::Settings[CUSTOM_LOG_PATH] = Settings::LogDirectoryPath;
        Settings::Save(SettingsPath);
    }

}