#define NOMINMAX
#include "gui.h"
#include "Settings.h"
#include "Shared.h"
#include "utils.h"
#include "resource.h"
#include "nexus/Nexus.h"
#include "mumble/Mumble.h"
#include "imgui/imgui.h"
#include <sstream>
#include <iomanip>
#include <algorithm>



void DrawBar(float frac, int count, uint64_t totalDamage, const ImVec4& color, const std::string& eliteSpec, bool showDamage, HINSTANCE hSelf)
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

void RenderSpecializationBars(const TeamStats& teamData, int teamIndex, HINSTANCE hSelf)
{
    bool useSquadStats = Settings::squadPlayersOnly && teamData.isPOVTeam;
    bool sortByDamage = Settings::sortSpecDamage;
    bool vsLogPlayers = Settings::vsLoggedPlayersOnly;
    bool showDamage = Settings::showSpecDamage;

    // Get the eliteSpecStats to display
    const std::unordered_map<std::string, SpecStats>& eliteSpecStatsToDisplay = useSquadStats ?
        teamData.squadStats.eliteSpecStats : teamData.eliteSpecStats;

    // Sort specializations by count or damage in descending order
    std::vector<std::pair<std::string, SpecStats>> sortedClasses;

    for (const auto& [eliteSpec, stats] : eliteSpecStatsToDisplay) {
        sortedClasses.emplace_back(eliteSpec, stats);
    }

    std::sort(sortedClasses.begin(), sortedClasses.end(),
        [sortByDamage, vsLogPlayers](const std::pair<std::string, SpecStats>& a, const std::pair<std::string, SpecStats>& b) {
            if (sortByDamage && !vsLogPlayers) {
                return a.second.totalDamage > b.second.totalDamage;
            }
            else if (sortByDamage && vsLogPlayers) {
                return a.second.totalDamageVsPlayers > b.second.totalDamageVsPlayers;
            }
            else {
                return a.second.count > b.second.count;
            }
        });

    uint64_t maxValue = 0;
    if (!sortedClasses.empty()) {
        if (sortByDamage && !vsLogPlayers) {
            maxValue = sortedClasses[0].second.totalDamage;
        }
        else if (sortByDamage && vsLogPlayers) {
            maxValue = sortedClasses[0].second.totalDamageVsPlayers;
        }
        else {
            maxValue = sortedClasses[0].second.count;
        }
    }

    for (const auto& specPair : sortedClasses) {
        const std::string& eliteSpec = specPair.first;
        const SpecStats& stats = specPair.second;
        int count = stats.count;

        uint64_t totalDamage;
        if (vsLogPlayers) {
            totalDamage = stats.totalDamageVsPlayers;
        }
        else {
            totalDamage = stats.totalDamage;
        }

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
        DrawBar(frac, count, totalDamage, color, eliteSpec, showDamage, hSelf);
    }
}


void RenderTeamData(int teamIndex, const TeamStats& teamData, HINSTANCE hSelf)
{
    ImGuiStyle& style = ImGui::GetStyle();
    float sz = ImGui::GetFontSize();

    ImGui::Spacing();

    // Determine whether to use squad stats or team stats
    bool useSquadStats = Settings::squadPlayersOnly && teamData.isPOVTeam;

    // Total Players
    uint32_t totalPlayersToDisplay = useSquadStats ? teamData.squadStats.totalPlayers : teamData.totalPlayers;

    if (Settings::showTeamTotalPlayers) {
        if (Settings::showClassIcons) {
            if (Squad && Squad->Resource) {
                ImGui::Image(Squad->Resource, ImVec2(sz, sz));
                ImGui::SameLine(0, 5);
            }
            else {
                Squad = APIDefs->GetTextureOrCreateFromResource("SQUAD_ICON", SQUAD, hSelf);
            }
        }
        if (Settings::showClassNames) {
            ImGui::Text("Total:  %d", totalPlayersToDisplay);
        }
        else {
            ImGui::Text("%d", totalPlayersToDisplay);
        }
    }

    uint32_t totalKillsToDisplay = useSquadStats ? teamData.squadStats.totalKills : teamData.totalKills;
    uint32_t totalDeathsfromKillingBlowsToDisplay = useSquadStats ? teamData.squadStats.totalDeathsFromKillingBlows : teamData.totalDeathsFromKillingBlows;
    double kdRatioToDisplay = useSquadStats ? teamData.squadStats.getKillDeathRatio() : teamData.getKillDeathRatio();
    std::string kdString = (std::ostringstream() << totalKillsToDisplay << "/" << totalDeathsfromKillingBlowsToDisplay << " (" << std::fixed << std::setprecision(2) << kdRatioToDisplay << ")").str();


    if (Settings::showTeamKDR) {
        if (Settings::showClassIcons) {
            if (Kdr && Kdr->Resource) {
                ImGui::Image(Kdr->Resource, ImVec2(sz, sz));
                ImGui::SameLine(0, 5);
            }
            else {
                Kdr = APIDefs->GetTextureOrCreateFromResource("KDR_ICON", KDR, hSelf);
            }
        }
        if (Settings::showClassNames) {
            ImGui::Text("K/D: %s", kdString.c_str());
        }
        else {
            ImGui::Text("%s", kdString.c_str());
        }
    }

    // Total Deaths
    uint32_t totalDeathsToDisplay = useSquadStats ? teamData.squadStats.totalDeaths : teamData.totalDeaths;

    if (Settings::showTeamDeaths) {
        if (Settings::showClassIcons) {
            if (Death && Death->Resource) {
                ImGui::Image(Death->Resource, ImVec2(sz, sz));
                ImGui::SameLine(0, 5);
            }
            else {
                Death = APIDefs->GetTextureOrCreateFromResource("DEATH_ICON", DEATH, hSelf);
            }
        }
        if (Settings::showClassNames) {
            ImGui::Text("Deaths: %d", totalDeathsToDisplay);
        }
        else {
            ImGui::Text("%d", totalDeathsToDisplay);
        }
    }

    // Total Downed
    uint32_t totalDownedToDisplay = useSquadStats ? teamData.squadStats.totalDowned : teamData.totalDowned;

    if (Settings::showTeamDowned) {
        if (Settings::showClassIcons) {
            if (Downed && Downed->Resource) {
                ImGui::Image(Downed->Resource, ImVec2(sz, sz));
                ImGui::SameLine(0, 5);
            }
            else {
                Downed = APIDefs->GetTextureOrCreateFromResource("DOWNED_ICON", DOWNED, hSelf);
            }
        }
        if (Settings::showClassNames) {
            ImGui::Text("Downs:  %d", totalDownedToDisplay);
        }
        else {
            ImGui::Text("%d", totalDownedToDisplay);
        }
    }

    // Total Damage
    uint64_t totalDamageToDisplay = 0;
    if (Settings::vsLoggedPlayersOnly) {
        totalDamageToDisplay = useSquadStats ? teamData.squadStats.totalDamageVsPlayers : teamData.totalDamageVsPlayers;
    }
    else {
        totalDamageToDisplay = useSquadStats ? teamData.squadStats.totalDamage : teamData.totalDamage;
    }

    if (Settings::showTeamDamage) {
        if (Settings::showClassIcons) {
            if (Damage && Damage->Resource) {
                ImGui::Image(Damage->Resource, ImVec2(sz, sz));
                ImGui::SameLine(0, 5);
            }
            else {
                Damage = APIDefs->GetTextureOrCreateFromResource("DAMAGE_ICON", DAMAGE, hSelf);
            }
        }

        std::string formattedDamage = formatDamage(totalDamageToDisplay);

        if (Settings::showClassNames) {
            ImGui::Text("Damage: %s", formattedDamage.c_str());
        }
        else {
            ImGui::Text("%s", formattedDamage.c_str());
        }
    }

    // Total Strike Damage
    uint64_t totalStrikeDamageToDisplay = 0;
    if (Settings::vsLoggedPlayersOnly) {
        totalStrikeDamageToDisplay = useSquadStats ? teamData.squadStats.totalStrikeDamageVsPlayers : teamData.totalStrikeDamageVsPlayers;
    }
    else {
        totalStrikeDamageToDisplay = useSquadStats ? teamData.squadStats.totalStrikeDamage : teamData.totalStrikeDamage;
    }

    if (Settings::showTeamStrikeDamage) {
        if (Settings::showClassIcons) {
            if (Strike && Strike->Resource) {
                ImGui::Image(Strike->Resource, ImVec2(sz, sz));
                ImGui::SameLine(0, 5);
            }
            else {
                Strike = APIDefs->GetTextureOrCreateFromResource("STRIKE_ICON", STRIKE, hSelf);
            }
        }

        std::string formattedDamage = formatDamage(totalStrikeDamageToDisplay);

        if (Settings::showClassNames) {
            ImGui::Text("Strike: %s", formattedDamage.c_str());
        }
        else {
            ImGui::Text("%s", formattedDamage.c_str());
        }
    }

    // Total Condition Damage
    uint64_t totalCondiDamageToDisplay = 0;
    if (Settings::vsLoggedPlayersOnly) {
        totalCondiDamageToDisplay = useSquadStats ? teamData.squadStats.totalCondiDamageVsPlayers : teamData.totalCondiDamageVsPlayers;
    }
    else {
        totalCondiDamageToDisplay = useSquadStats ? teamData.squadStats.totalCondiDamage : teamData.totalCondiDamage;
    }

    if (Settings::showTeamCondiDamage) {
        if (Settings::showClassIcons) {
            if (Condi && Condi->Resource) {
                ImGui::Image(Condi->Resource, ImVec2(sz, sz));
                ImGui::SameLine(0, 5);
            }
            else {
                Condi = APIDefs->GetTextureOrCreateFromResource("CONDI_ICON", CONDI, hSelf);
            }
        }

        std::string formattedDamage = formatDamage(totalCondiDamageToDisplay);

        if (Settings::showClassNames) {
            ImGui::Text("Condi:  %s", formattedDamage.c_str());
        }
        else {
            ImGui::Text("%s", formattedDamage.c_str());
        }
    }

    // Display specialization bars only if not in split window mode
    if (Settings::showSpecBars && !Settings::splitStatsWindow) {
        ImGui::Separator();

        bool sortByDamage = Settings::sortSpecDamage;
        bool vsLogPlayers = Settings::vsLoggedPlayersOnly;
        bool showDamage = Settings::showSpecDamage;

        // Get the eliteSpecStats to display
        const std::unordered_map<std::string, SpecStats>& eliteSpecStatsToDisplay = useSquadStats ?
            teamData.squadStats.eliteSpecStats : teamData.eliteSpecStats;

        // Sort specializations by count or damage in descending order
        std::vector<std::pair<std::string, SpecStats>> sortedClasses;

        for (const auto& [eliteSpec, stats] : eliteSpecStatsToDisplay) {
            sortedClasses.emplace_back(eliteSpec, stats);
        }

        std::sort(sortedClasses.begin(), sortedClasses.end(),
            [sortByDamage, vsLogPlayers](const std::pair<std::string, SpecStats>& a, const std::pair<std::string, SpecStats>& b) {
                if (sortByDamage && !vsLogPlayers) {
                    return a.second.totalDamage > b.second.totalDamage;
                }
                else if (sortByDamage && vsLogPlayers) {
                    return a.second.totalDamageVsPlayers > b.second.totalDamageVsPlayers;
                }
                else {
                    return a.second.count > b.second.count;
                }
            });

        uint64_t maxValue = 0;
        if (!sortedClasses.empty()) {
            if (sortByDamage && !vsLogPlayers) {
                maxValue = sortedClasses[0].second.totalDamage;
            }
            else if (sortByDamage && vsLogPlayers) {
                maxValue = sortedClasses[0].second.totalDamageVsPlayers;
            }
            else {
                maxValue = sortedClasses[0].second.count;
            }
        }

        for (const auto& specPair : sortedClasses) {
            const std::string& eliteSpec = specPair.first;
            const SpecStats& stats = specPair.second;
            int count = stats.count;

            uint64_t totalDamage;
            if (vsLogPlayers) {
                totalDamage = stats.totalDamageVsPlayers;
            }
            else {
                totalDamage = stats.totalDamage;
            }

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
            DrawBar(frac, count, totalDamage, color, eliteSpec, showDamage, hSelf);
        }
    }
}


// Updated RenderSimpleRatioBar to accept dynamic data
void RenderSimpleRatioBar(
    const std::vector<float>& counts,
    const std::vector<ImVec4>& colors,
    const ImVec2& size,
    const std::vector<const char*>& texts)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 p = ImGui::GetCursorScreenPos();

    size_t numTeams = counts.size();

    // Calculate total count
    float total = 0.0f;
    for (float count : counts)
        total += count;

    // Calculate fractions
    std::vector<float> fractions(numTeams);
    if (total == 0.0f)
    {
        // All counts are zero, distribute the bar equally among present teams
        float equalFrac = 1.0f / numTeams;
        for (size_t i = 0; i < numTeams; ++i)
            fractions[i] = equalFrac;
    }
    else
    {
        for (size_t i = 0; i < numTeams; ++i)
            fractions[i] = counts[i] / total;
    }

    // Convert colors to ImU32
    std::vector<ImU32> colorsU32(numTeams);
    for (size_t i = 0; i < numTeams; ++i)
        colorsU32[i] = ImGui::ColorConvertFloat4ToU32(colors[i]);

    // Base coordinates and dimensions
    const float x = p.x;
    const float y = p.y;
    const float width = size.x;
    const float height = size.y;

    // Draw rectangles and texts
    float x_start = x;
    for (size_t i = 0; i < numTeams; ++i)
    {
        float section_width = width * fractions[i];
        float x_end = x_start + section_width;

        // Draw rectangle
        draw_list->AddRectFilled(
            ImVec2(x_start, y),
            ImVec2(x_end, y + height),
            colorsU32[i]
        );

        // Calculate text dimensions and positions
        ImVec2 textSize = ImGui::CalcTextSize(texts[i]);
        float text_center_x = x_start + (section_width - textSize.x) * 0.5f + Settings::widgetTextHorizontalAlignOffset;
        float center_y = y + (height - textSize.y) * 0.5f + Settings::widgetTextVerticalAlignOffset;

        // Draw text if there's enough space
        if (section_width >= textSize.x)
        {
            draw_list->AddText(
                ImVec2(text_center_x, center_y),
                IM_COL32_WHITE,
                texts[i]
            );
        }

        x_start = x_end;
    }

    // Draw white border around the entire bar
    draw_list->AddRect(
        ImVec2(x, y),
        ImVec2(x + width, y + height),
        IM_COL32_WHITE
    );

    ImGui::Dummy(size);
}

// Full updated ratioBarSetup function
void ratioBarSetup()
{
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;
    if (Settings::disableMovingWindow)
    {
        window_flags |= ImGuiWindowFlags_NoMove;
    }
    if (Settings::disableClickingWindow)
    {
        window_flags |= ImGuiWindowFlags_NoInputs;
    }
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

    ImVec2 barSize = ImVec2(Settings::widgetWidth, Settings::widgetHeight);
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

        // Define team names and colors
        const std::vector<std::string> team_names = { "Red", "Blue", "Green" };
        const std::vector<ImVec4> team_colors = {
            ImGui::ColorConvertU32ToFloat4(IM_COL32(0xff, 0x44, 0x44, 0xff)), // Red
            ImGui::ColorConvertU32ToFloat4(IM_COL32(0x33, 0xb5, 0xe5, 0xff)), // Blue
            ImGui::ColorConvertU32ToFloat4(IM_COL32(0x99, 0xcc, 0x00, 0xff))  // Green
        };

        // Structure to hold data for teams to display
        struct TeamDisplayData
        {
            float count;
            ImVec4 color;
            std::string text;
        };

        std::vector<TeamDisplayData> teamsToDisplay;

        for (size_t i = 0; i < team_names.size(); ++i)
        {
            auto teamIt = currentLogData.teamStats.find(team_names[i]);
            if (teamIt != currentLogData.teamStats.end())
            {
                bool useSquadStats = Settings::squadPlayersOnly && teamIt->second.isPOVTeam;
                Settings::widgetStats = Settings::widgetStatsC;

                float teamCountValue = 0.0f;
                if (Settings::widgetStats == "players") {
                    teamCountValue = static_cast<float>(useSquadStats ? teamIt->second.squadStats.totalPlayers : teamIt->second.totalPlayers);
                }
                else if (Settings::widgetStats == "deaths") {
                    teamCountValue = static_cast<float>(useSquadStats ? teamIt->second.squadStats.totalDeaths : teamIt->second.totalDeaths);
                }
                else if (Settings::widgetStats == "downs") {
                    teamCountValue = static_cast<float>(useSquadStats ? teamIt->second.squadStats.totalDowned : teamIt->second.totalDowned);
                }
                else if (Settings::widgetStats == "damage") {
                    float totalDamageToDisplay;
                    if (Settings::vsLoggedPlayersOnly) {
                        totalDamageToDisplay = static_cast<float>(useSquadStats ?
                            teamIt->second.squadStats.totalDamageVsPlayers :
                            teamIt->second.totalDamageVsPlayers);
                    }
                    else {
                        totalDamageToDisplay = static_cast<float>(useSquadStats ?
                            teamIt->second.squadStats.totalDamage :
                            teamIt->second.totalDamage);
                    }
                    teamCountValue = totalDamageToDisplay;
                }
                else if (Settings::widgetStats == "kdr") {
                    float kdRatioToDisplay = useSquadStats ? teamIt->second.squadStats.getKillDeathRatio() : teamIt->second.getKillDeathRatio();
                    teamCountValue = kdRatioToDisplay;
                }

                char buf[32];
                if (Settings::widgetStats == "damage") {
                    strcpy_s(buf, sizeof(buf), formatDamage(static_cast<uint64_t>(teamCountValue)).c_str());
                }
                else if (Settings::widgetStats == "kdr") {
                    snprintf(buf, sizeof(buf), "%.2f", teamCountValue);
                }
                else {
                    snprintf(buf, sizeof(buf), "%.0f", teamCountValue);
                }

                teamsToDisplay.push_back(TeamDisplayData{
                    teamCountValue,
                    team_colors[i],
                    buf
                    });
            }
        }

        if (teamsToDisplay.empty())
        {
            ImGui::Text("No team data available.");
            ImGui::End();
            ImGui::PopStyleVar(4);
            return;
        }

        std::vector<float> counts;
        std::vector<ImVec4> colors;
        std::vector<const char*> texts;

        for (const auto& teamData : teamsToDisplay)
        {
            counts.push_back(teamData.count);
            colors.push_back(teamData.color);
            texts.push_back(teamData.text.c_str());
        }

        RenderSimpleRatioBar(
            counts,
            colors,
            ImVec2(barSize.x, barSize.y),
            texts
        );
    }
    ImGui::PopStyleVar(4);

    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_ChildWindows) && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        ImGui::OpenPopup("Widget Menu");
    }
    if (ImGui::BeginPopup("Widget Menu"))
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

        if (ImGui::BeginMenu("Display Stats"))
        {
            Settings::widgetStats = Settings::widgetStatsC;
            bool isPlayers = Settings::widgetStats == "players";
            bool isDeaths = Settings::widgetStats == "deaths";
            bool isDowns = Settings::widgetStats == "downs";
            bool isDamage = Settings::widgetStats == "damage";
            bool isKDR = Settings::widgetStats == "kdr";

            if (ImGui::RadioButton("Players", isPlayers))
            {
                strcpy_s(Settings::widgetStatsC, sizeof(Settings::widgetStatsC), "players");
                Settings::widgetStats = "players";
                Settings::Settings[WIDGET_STATS] = Settings::widgetStatsC;
                Settings::Save(APIDefs->GetAddonDirectory("WvWFightAnalysis/settings.json"));
            }
            if (ImGui::RadioButton("K/D Ratio", isKDR))
            {
                strcpy_s(Settings::widgetStatsC, sizeof(Settings::widgetStatsC), "kdr");
                Settings::widgetStats = "kdr";
                Settings::Settings[WIDGET_STATS] = Settings::widgetStatsC;
                Settings::Save(APIDefs->GetAddonDirectory("WvWFightAnalysis/settings.json"));
            }
            if (ImGui::RadioButton("Deaths", isDeaths))
            {
                strcpy_s(Settings::widgetStatsC, sizeof(Settings::widgetStatsC), "deaths");
                Settings::widgetStats = "deaths";
                Settings::Settings[WIDGET_STATS] = Settings::widgetStatsC;
                Settings::Save(APIDefs->GetAddonDirectory("WvWFightAnalysis/settings.json"));
            }
            if (ImGui::RadioButton("Downs", isDowns))
            {
                strcpy_s(Settings::widgetStatsC, sizeof(Settings::widgetStatsC), "downs");
                Settings::widgetStats = "downs";
                Settings::Settings[WIDGET_STATS] = Settings::widgetStatsC;
                Settings::Save(APIDefs->GetAddonDirectory("WvWFightAnalysis/settings.json"));
            }
            if (ImGui::RadioButton("Damage", isDamage))
            {
                strcpy_s(Settings::widgetStatsC, sizeof(Settings::widgetStatsC), "damage");
                Settings::widgetStats = "damage";
                Settings::Settings[WIDGET_STATS] = Settings::widgetStatsC;
                Settings::Save(APIDefs->GetAddonDirectory("WvWFightAnalysis/settings.json"));
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Style"))
        {
            // Widget Height
            ImGui::SetNextItemWidth(200.0f);
            if (ImGui::SliderFloat("##Widget Height", &Settings::widgetHeight, 0.0f, 900.0f))
            {
                Settings::widgetHeight = std::clamp(Settings::widgetHeight, 0.0f, 900.0f);
                Settings::Settings[WIDGET_HEIGHT] = Settings::widgetHeight;
                Settings::Save(APIDefs->GetAddonDirectory("WvWFightAnalysis/settings.json"));
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(60.0f);
            if (ImGui::InputFloat("Widget Height", &Settings::widgetHeight, 1.0f))
            {
                Settings::widgetHeight = std::clamp(Settings::widgetHeight, 0.0f, 900.0f);
                Settings::Settings[WIDGET_HEIGHT] = Settings::widgetHeight;
                Settings::Save(APIDefs->GetAddonDirectory("WvWFightAnalysis/settings.json"));
            }

            // Widget Width
            ImGui::SetNextItemWidth(200.0f);
            if (ImGui::SliderFloat("##Widget Width", &Settings::widgetWidth, 0.0f, 900.0f))
            {
                Settings::widgetWidth = std::clamp(Settings::widgetWidth, 0.0f, 900.0f);
                Settings::Settings[WIDGET_WIDTH] = Settings::widgetWidth;
                Settings::Save(APIDefs->GetAddonDirectory("WvWFightAnalysis/settings.json"));
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(60.0f);
            if (ImGui::InputFloat("Widget Width", &Settings::widgetWidth, 1.0f))
            {
                Settings::widgetWidth = std::clamp(Settings::widgetWidth, 0.0f, 900.0f);
                Settings::Settings[WIDGET_WIDTH] = Settings::widgetWidth;
                Settings::Save(APIDefs->GetAddonDirectory("WvWFightAnalysis/settings.json"));
            }

            // Vertical Align
            ImGui::SetNextItemWidth(200.0f);
            if (ImGui::SliderFloat("##Widget Text Vertical Align", &Settings::widgetTextVerticalAlignOffset, -50.0f, 50.0f))
            {
                Settings::widgetTextVerticalAlignOffset = std::clamp(Settings::widgetTextVerticalAlignOffset, -50.0f, 50.0f);
                Settings::Settings[WIDGET_TEXT_VERTICAL_OFFSET] = Settings::widgetTextVerticalAlignOffset;
                Settings::Save(APIDefs->GetAddonDirectory("WvWFightAnalysis/settings.json"));
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(60.0f);
            if (ImGui::InputFloat("Widget Text Vertical Align", &Settings::widgetTextVerticalAlignOffset, 1.0f))
            {
                Settings::widgetTextVerticalAlignOffset = std::clamp(Settings::widgetTextVerticalAlignOffset, -50.0f, 50.0f);
                Settings::Settings[WIDGET_TEXT_VERTICAL_OFFSET] = Settings::widgetTextVerticalAlignOffset;
                Settings::Save(APIDefs->GetAddonDirectory("WvWFightAnalysis/settings.json"));
            }

            // Horizontal Align
            ImGui::SetNextItemWidth(200.0f);
            if (ImGui::SliderFloat("##Widget Text Horizontal Align", &Settings::widgetTextHorizontalAlignOffset, -50.0f, 50.0f))
            {
                Settings::widgetTextHorizontalAlignOffset = std::clamp(Settings::widgetTextHorizontalAlignOffset, -50.0f, 50.0f);
                Settings::Settings[WIDGET_TEXT_HORIZONTAL_OFFSET] = Settings::widgetTextHorizontalAlignOffset;
                Settings::Save(APIDefs->GetAddonDirectory("WvWFightAnalysis/settings.json"));
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(60.0f);
            if (ImGui::InputFloat("Widget Text Horizontal Align", &Settings::widgetTextHorizontalAlignOffset, 1.0f))
            {
                Settings::widgetTextHorizontalAlignOffset = std::clamp(Settings::widgetTextHorizontalAlignOffset, -50.0f, 50.0f);
                Settings::Settings[WIDGET_TEXT_HORIZONTAL_OFFSET] = Settings::widgetTextHorizontalAlignOffset;
                Settings::Save(APIDefs->GetAddonDirectory("WvWFightAnalysis/settings.json"));
            }
            ImGui::EndMenu();
        }
        if (ImGui::Checkbox("Damage vs Logged Players Only", &Settings::vsLoggedPlayersOnly))
        {
            Settings::Settings[VS_LOGGED_PLAYERS_ONLY] = Settings::vsLoggedPlayersOnly;
            Settings::Save(APIDefs->GetAddonDirectory("WvWFightAnalysis/settings.json"));
        }
        if (ImGui::Checkbox("Show Squad Players Only", &Settings::squadPlayersOnly))
        {
            Settings::Settings[SQUAD_PLAYERS_ONLY] = Settings::squadPlayersOnly;
            Settings::Save(APIDefs->GetAddonDirectory("WvWFightAnalysis/settings.json"));
        }

    }
    ImGui::End();
}


