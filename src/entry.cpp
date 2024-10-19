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



void DrawBar(float frac, int count, uint64_t totalDamage, const ImVec4& color, const std::string& eliteSpec, bool showDamage)
{
    ImVec2 cursor_pos = ImGui::GetCursorPos();
    ImVec2 screen_pos = ImGui::GetCursorScreenPos();
    float bar_width = ImGui::GetContentRegionAvail().x * frac;
    float bar_height = ImGui::GetTextLineHeight() + 4;

    ImGui::GetWindowDrawList()->AddRectFilled(
        screen_pos,
        ImVec2(screen_pos.x + bar_width, screen_pos.y + bar_height),
        ImGui::ColorConvertFloat4ToU32(color)
    );

    ImGui::SetCursorPos(ImVec2(cursor_pos.x + 5, cursor_pos.y + 2));

    ImGui::Text("%d", count);

    ImGui::SameLine(0, 5);

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
                    ImGui::Text("%c%c", eliteSpec[0], eliteSpec[1]);
                }
            }
            else
            {
                ImGui::Text("%c%c", eliteSpec[0], eliteSpec[1]);
            }
        }
        ImGui::SameLine(0, 5);
    }


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
                ImGui::Text("%s", shortClassName.c_str());
            }
        }
        else
        {
            ImGui::Text("%s", eliteSpec.c_str());
        }
    }
    else
    {
        ImGui::Text(" ");
    }
    if (showDamage) {
        ImGui::SameLine(0, 5);

        std::string formattedDamage = formatDamage(static_cast<double>(totalDamage));
        ImGui::Text("(%s)", formattedDamage.c_str());
    }


    ImGui::SetCursorPosY(cursor_pos.y + bar_height + 2);
}

void RenderSimpleRatioBar(int red, int green, int blue,
    const ImVec4& colorRed, const ImVec4& colorGreen, const ImVec4& colorBlue,
    const ImVec2& size)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 p = ImGui::GetCursorScreenPos();


    float total = static_cast<float>(red + green + blue);
    if (total == 0.0f) total = 1.0f;
    float r_frac = static_cast<float>(red) / total;
    float g_frac = static_cast<float>(green) / total;
    float b_frac = static_cast<float>(blue) / total;


    ImU32 colRed = ImGui::ColorConvertFloat4ToU32(colorRed);
    ImU32 colGreen = ImGui::ColorConvertFloat4ToU32(colorGreen);
    ImU32 colBlue = ImGui::ColorConvertFloat4ToU32(colorBlue);


    float x = p.x;
    float y = p.y;
    float width = size.x;
    float height = size.y;


    float r_width = width * r_frac;
    float g_width = width * g_frac;
    float b_width = width * b_frac;


    float x_red_start = x;
    float x_red_end = x + r_width;

    float x_green_start = x_red_end;
    float x_green_end = x_green_start + g_width;

    float x_blue_start = x_green_end;
    float x_blue_end = x + width;


    draw_list->AddRectFilled(ImVec2(x_red_start, y), ImVec2(x_red_end, y + height), colRed);
    draw_list->AddRectFilled(ImVec2(x_green_start, y), ImVec2(x_green_end, y + height), colGreen);
    draw_list->AddRectFilled(ImVec2(x_blue_start, y), ImVec2(x_blue_end, y + height), colBlue);

    draw_list->AddRect(ImVec2(x, y), ImVec2(x + width, y + height), IM_COL32_WHITE);

    ImGuiIO& io = ImGui::GetIO();
    ImFont* font = io.FontDefault;

    char bufRed[32], bufGreen[32], bufBlue[32];
    snprintf(bufRed, sizeof(bufRed), "%d", red);
    snprintf(bufGreen, sizeof(bufGreen), "%d", green);
    snprintf(bufBlue, sizeof(bufBlue), "%d", blue);

    ImVec2 textSizeRed = font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.0f, bufRed);
    ImVec2 textSizeGreen = font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.0f, bufGreen);
    ImVec2 textSizeBlue = font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.0f, bufBlue);

    float red_center_x = x_red_start + r_width / 2.0f - textSizeRed.x / 2.0f;
    float green_center_x = x_green_start + g_width / 2.0f - textSizeGreen.x / 2.0f;
    float blue_center_x = x_blue_start + b_width / 2.0f - textSizeBlue.x / 2.0f;

    float center_y = y + height / 2.0f - textSizeRed.y / 2.0f;

    if (r_width >= textSizeRed.x)
    {
        draw_list->AddText(ImVec2(red_center_x, center_y), IM_COL32_WHITE, bufRed);
    }
    if (g_width >= textSizeGreen.x)
    {
        draw_list->AddText(ImVec2(green_center_x, center_y), IM_COL32_WHITE, bufGreen);
    }
    if (b_width >= textSizeBlue.x)
    {
        draw_list->AddText(ImVec2(blue_center_x, center_y), IM_COL32_WHITE, bufBlue);
    }

    ImGui::Dummy(size);
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
    AddonDef.Version.Revision = 9;
    AddonDef.Author = "Unreal";
    AddonDef.Description = "Simple WvW log analysis tool.";
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

    AddonPath = APIDefs->GetAddonDirectory("WvWFightAnalysis");
    SettingsPath = APIDefs->GetAddonDirectory("WvWFightAnalysis/settings.json");
    std::filesystem::create_directory(AddonPath);
    Settings::Load(SettingsPath);

    APIDefs->RegisterKeybindWithString("KB_MI_TOGGLEVISIBLE", ProcessKeybinds, "(null)");

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
            else if (ImGui::BeginTable("TeamTable", teamsWithData, table_flags))
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
                            if (Settings::showTeamTotalPlayers) {
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
                            }
                            if (Settings::showTeamDeaths) {
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
                            }
                            if (Settings::showTeamDowned) {
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
                            }
                            if (Settings::showTeamDamage) {
                                if (Settings::showClassIcons)
                                {
                                    if (Damage && Damage->Resource)
                                    {
                                        ImGui::Image(Damage->Resource, ImVec2(sz, sz));
                                        ImGui::SameLine(0, 5);
                                    }
                                    else
                                    {
                                        Damage = APIDefs->GetTextureOrCreateFromResource("DAMAGE_ICON", DAMAGE, hSelf);
                                    }
                                }
                                std::string formattedDamage = formatDamage(teamData.totalDamage);
                                if (Settings::showClassNames)
                                {

                                    ImGui::Text("Damage: %s", formattedDamage.c_str());
                                }
                                else
                                {
                                    ImGui::Text("%s", formattedDamage.c_str());
                                }
                            }
                            ImGui::Separator();

                            bool sortByDamage = Settings::sortSpecDamage;
                            bool showDamage = Settings::showSpecDamage;

                            // Sort classes by count or damage in descending order
                            std::vector<std::pair<std::string, SpecStats>> sortedClasses;

                            for (const auto& [eliteSpec, stats] : teamData.eliteSpecStats) {
                                sortedClasses.emplace_back(eliteSpec, stats);
                            }

                            std::sort(sortedClasses.begin(), sortedClasses.end(),
                                [sortByDamage](const std::pair<std::string, SpecStats>& a, const std::pair<std::string, SpecStats>& b) {
                                    if (sortByDamage) {
                                        return a.second.totalDamage > b.second.totalDamage;
                                    }
                                    else {
                                        return a.second.count > b.second.count;
                                    }
                                });

                            uint64_t maxValue = 0;
                            if (!sortedClasses.empty()) {
                                if (sortByDamage) {
                                    maxValue = sortedClasses[0].second.totalDamage;
                                }
                                else {
                                    maxValue = sortedClasses[0].second.count;
                                }
                            }

                            for (const auto& specPair : sortedClasses)
                            {
                                const std::string& eliteSpec = specPair.first;
                                const SpecStats& stats = specPair.second;
                                int count = stats.count;
                                uint64_t totalDamage = stats.totalDamage;

                                // Determine the value and fraction based on the sorting criterion
                                uint64_t value = sortByDamage ? totalDamage : count;
                                float frac = (maxValue > 0) ? static_cast<float>(value) / maxValue : 0.0f;

                                // Get the profession name
                                std::string professionName;
                                auto it = eliteSpecToProfession.find(eliteSpec);
                                if (it != eliteSpecToProfession.end()) {
                                    professionName = it->second;
                                }
                                else {
                                    professionName = "Unknown";
                                }

                                // Get the color for the profession
                                ImVec4 color;
                                auto colorIt = professionColors.find(professionName);
                                if (colorIt != professionColors.end()) {
                                    color = colorIt->second;
                                }
                                else {
                                    color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                                }

                                // Draw the bar with the updated parameters
                                DrawBar(frac, count, totalDamage, color, eliteSpec, showDamage);
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

                        std::string fnstr = log.filename.substr(0, log.filename.find_last_of('.'));
                        uint64_t durationMs = log.data.combatEndTime - log.data.combatStartTime;
                        auto duration = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::milliseconds(durationMs));
                        int minutes = duration.count() / 60;
                        int seconds = duration.count() % 60;
                        std::string displayName = fnstr + " (" + std::to_string(minutes) + "m" + std::to_string(seconds) + "s)";

                        if (ImGui::RadioButton(displayName.c_str(), &currentLogIndex, i))
                        {
                            // Your existing selection logic here
                        }
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Display")) {
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
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Style")) {
                    if (ImGui::Checkbox("Show Scroll Bar", &Settings::showScrollBar))
                    {
                        Settings::Settings[SHOW_SCROLL_BAR] = Settings::showScrollBar;
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