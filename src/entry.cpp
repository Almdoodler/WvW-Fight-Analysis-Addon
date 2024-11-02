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
    AddonDef.Version.Revision = 6;
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

    directoryMonitorThread = std::thread(monitorDirectory, Settings::logHistorySize, Settings::pollIntervalMilliseconds);

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
        currentLogIndex = 0;
    }

    if (!NexusLink || !NexusLink->IsGameplay || !MumbleLink || MumbleLink->Context.IsMapOpen)
    {
        return;
    }

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

    if (MumbleLink->Context.IsInCombat && !Settings::showWindowInCombat)
    {
        return;
    }

    if (Settings::IsAddonWidgetEnabled)
    {
        ratioBarSetup();
    }

    if (Settings::IsAddonWindowEnabled)
    {
        ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse;

        if (!Settings::showScrollBar) window_flags |= ImGuiWindowFlags_NoScrollbar;

        if (!Settings::showWindowTitle)
        {
            window_flags |= ImGuiWindowFlags_NoTitleBar;
        }
        if (Settings::disableMovingWindow)
        {
            window_flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
        }
        if (Settings::disableClickingWindow)
        {
            window_flags |= ImGuiWindowFlags_NoInputs;
        }

        if (ImGui::Begin("WvW Fight Analysis", nullptr, window_flags))
        {
            ImGuiStyle& style = ImGui::GetStyle();
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(style.FramePadding.x, 5));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x, 10));

            if (parsedLogs.empty())
            {
                ImGui::Text(initialParsingComplete ? "No logs parsed yet." : "Parsing logs...");
                ImGui::PopStyleVar(2);
                ImGui::End();
                return;
            }

            const auto& currentLog = parsedLogs[currentLogIndex];
            std::string fnstr = currentLog.filename.substr(0, currentLog.filename.find_last_of('.'));
            uint64_t durationMs = currentLog.data.combatEndTime - currentLog.data.combatStartTime;
            auto duration = std::chrono::milliseconds(durationMs);
            int minutes = std::chrono::duration_cast<std::chrono::minutes>(duration).count();
            int seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count() % 60;
            std::string displayName = fnstr + " (" + std::to_string(minutes) + "m " + std::to_string(seconds) + "s)";

            ImGui::Text("%s", displayName.c_str());

            const auto& currentLogData = currentLog.data;
            const char* team_names[] = { "Red", "Blue", "Green" };
            ImVec4 team_colors[] = {
                ImGui::ColorConvertU32ToFloat4(IM_COL32(0xFF, 0x44, 0x44, 0xFF)),
                ImGui::ColorConvertU32ToFloat4(IM_COL32(0x33, 0xB5, 0xE5, 0xFF)),
                ImGui::ColorConvertU32ToFloat4(IM_COL32(0x99, 0xCC, 0x00, 0xFF))
            };

            int teamsWithData = 0;
            bool teamHasData[3] = { false, false, false };
            for (int i = 0; i < 3; ++i)
            {
                auto teamIt = currentLogData.teamStats.find(team_names[i]);
                if (teamIt != currentLogData.teamStats.end() && teamIt->second.totalPlayers >= Settings::teamPlayerThreshold)
                {
                    teamsWithData++;
                    teamHasData[i] = true;
                }
            }

            if (teamsWithData == 0)
            {
                ImGui::Text("No team data available meeting the player threshold.");
            }
            else
            {
                if (Settings::useTabbedView)
                {
                    if (ImGui::BeginTabBar("TeamTabBar", ImGuiTabBarFlags_None))
                    {
                        for (int i = 0; i < 3; ++i)
                        {
                            if (teamHasData[i])
                            {
                                ImGui::PushStyleColor(ImGuiCol_Text, team_colors[i]);
                                if (ImGui::BeginTabItem(team_names[i]))
                                {
                                    ImGui::PopStyleColor();

                                    const auto& teamData = currentLogData.teamStats.at(team_names[i]);
                                    RenderTeamData(i, teamData, hSelf);

                                    ImGui::EndTabItem();
                                }
                                else
                                {
                                    ImGui::PopStyleColor();
                                }
                            }
                        }
                        ImGui::EndTabBar();
                    }
                }
                else
                {
                    //ImVec2 table_size = ImVec2(0.0f, ImGui::GetContentRegionAvail().y);
                    if (ImGui::BeginTable("TeamTable", teamsWithData, ImGuiTableFlags_BordersInner))
                    {

                        for (int i = 0; i < 3; ++i)
                        {
                            if (teamHasData[i])
                            {
                                ImGui::TableSetupColumn(team_names[i], ImGuiTableColumnFlags_WidthStretch);
                            }
                        }

                        ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
                        int columnIndex = 0;
                        for (int i = 0; i < 3; ++i)
                        {
                            if (teamHasData[i])
                            {
                                ImGui::TableSetColumnIndex(columnIndex++);
                                ImGui::PushStyleColor(ImGuiCol_Text, team_colors[i]);
                                ImGui::Text("%s Team", team_names[i]);
                                ImGui::PopStyleColor();
                            }
                        }

                        ImGui::TableNextRow();
                        columnIndex = 0;
                        for (int i = 0; i < 3; ++i)
                        {
                            if (teamHasData[i])
                            {
                                ImGui::TableSetColumnIndex(columnIndex++);
                                const auto& teamData = currentLogData.teamStats.at(team_names[i]);
                                RenderTeamData(i, teamData, hSelf);
                            }
                        }

                        ImGui::EndTable();
                    }

                }
            }

            ImGui::PopStyleVar(2);

            if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_ChildWindows) && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
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
                        auto duration = std::chrono::milliseconds(durationMs);
                        int minutes = std::chrono::duration_cast<std::chrono::minutes>(duration).count();
                        int seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count() % 60;
                        std::string displayName = fnstr + " (" + std::to_string(minutes) + "m " + std::to_string(seconds) + "s)";

                        if (ImGui::RadioButton(displayName.c_str(), &currentLogIndex, i))
                        {
                            // Selection handled by RadioButton
                        }
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Display"))
                {
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
                if (ImGui::BeginMenu("Style"))
                {
                    if (ImGui::Checkbox("Use Tabbed View", &Settings::useTabbedView))
                    {
                        Settings::Settings[USE_TABBED_VIEW] = Settings::useTabbedView;
                        Settings::Save(SettingsPath);
                    }
                    if (ImGui::Checkbox("Show Scroll Bar", &Settings::showScrollBar))
                    {
                        Settings::Settings[SHOW_SCROLL_BAR] = Settings::showScrollBar;
                        Settings::Save(SettingsPath);
                    }
                    if (ImGui::Checkbox("Show Title", &Settings::showWindowTitle))
                    {
                        Settings::Settings[SHOW_WINDOW_TITLE] = Settings::showWindowTitle;
                        Settings::Save(SettingsPath);
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndPopup();
            }

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
    if (ImGui::Checkbox("Lock Window & Widget Position##WvWFightAnalysis", &Settings::disableMovingWindow))
    {
        Settings::Settings[DISABLE_MOVING_WINDOW] = Settings::disableMovingWindow;
        Settings::Save(SettingsPath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Disables moving & resizing.");
        ImGui::EndTooltip();
    }
    if (ImGui::Checkbox("Enable Mouse-Through##WvWFightAnalysis", &Settings::disableClickingWindow))
    {
        Settings::Settings[DISABLE_CLICKING_WINDOW] = Settings::disableClickingWindow;
        Settings::Save(SettingsPath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Window cannot be interacted with via mouse.");
        ImGui::EndTooltip();
    }
    if (ImGui::Checkbox("Enable Wine Compatibility Mode##WvWFightAnalysis", &Settings::forceLinuxCompatibilityMode))
    {
        Settings::Settings[FORCE_LINUX_COMPAT] = Settings::forceLinuxCompatibilityMode;
        Settings::Save(SettingsPath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Wine / Proton doesn't support ReadDirectoryChangesW, use directory polling instead.");
        ImGui::EndTooltip();
    }
    int tempPollIntervalMilliseconds = static_cast<int>(Settings::pollIntervalMilliseconds);
    if (ImGui::InputInt("ms Polling Interval##WvWFightAnalysis", &tempPollIntervalMilliseconds))
    {
        Settings::pollIntervalMilliseconds = static_cast<size_t>(std::clamp(tempPollIntervalMilliseconds, 500, 10000));
        Settings::Settings[POLL_INTERVAL_MILLISECONDS] = Settings::pollIntervalMilliseconds;
        Settings::Save(SettingsPath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Polling Interval when using Wine compatibility mode.");
        ImGui::EndTooltip();
    }
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