#include "gui.h"
#include "Settings.h"
#include "Shared.h"
#include "utils.h"
#include "resource.h"
#include "nexus/Nexus.h"
#include "mumble/Mumble.h"
#include "imgui/imgui.h"

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


    char bufRed[32], bufGreen[32], bufBlue[32];
    snprintf(bufRed, sizeof(bufRed), "%d", red);
    snprintf(bufGreen, sizeof(bufGreen), "%d", green);
    snprintf(bufBlue, sizeof(bufBlue), "%d", blue);

    ImVec2 textSizeRed = ImGui::CalcTextSize(bufRed);
    ImVec2 textSizeGreen = ImGui::CalcTextSize(bufGreen);
    ImVec2 textSizeBlue = ImGui::CalcTextSize(bufBlue);

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

void RenderTeamData(int teamIndex, const TeamStats& teamData, HINSTANCE hSelf)
{
    ImGuiStyle& style = ImGui::GetStyle();
    float sz = ImGui::GetFontSize();

    ImGui::Spacing();

    // Display team total players
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

    // Display team deaths
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

    // Display team downed
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

    // Display team total damage
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
        std::string formattedDamage;
        if (Settings::vsLoggedPlayersOnly) {
            formattedDamage = formatDamage(teamData.totalDamageVsPlayers);
        }
        else {
            formattedDamage = formatDamage(teamData.totalDamage);
        }
        if (Settings::showClassNames)
        {
            ImGui::Text("Damage: %s", formattedDamage.c_str());
        }
        else
        {
            ImGui::Text("%s", formattedDamage.c_str());
        }
    }

    // Display team strike damage
    if (Settings::showTeamStrikeDamage) {
        if (Settings::showClassIcons)
        {
            if (Strike && Strike->Resource)
            {
                ImGui::Image(Strike->Resource, ImVec2(sz, sz));
                ImGui::SameLine(0, 5);
            }
            else
            {
                Strike = APIDefs->GetTextureOrCreateFromResource("STRIKE_ICON", STRIKE, hSelf);
            }
        }
        std::string formattedDamage;
        if (Settings::vsLoggedPlayersOnly) {
            formattedDamage = formatDamage(teamData.totalStrikeDamageVsPlayers);
        }
        else {
            formattedDamage = formatDamage(teamData.totalStrikeDamage);
        }

        if (Settings::showClassNames)
        {
            ImGui::Text("Strike: %s", formattedDamage.c_str());
        }
        else
        {
            ImGui::Text("%s", formattedDamage.c_str());
        }
    }

    // Display team condi damage
    if (Settings::showTeamCondiDamage) {
        if (Settings::showClassIcons)
        {
            if (Condi && Condi->Resource)
            {
                ImGui::Image(Condi->Resource, ImVec2(sz, sz));
                ImGui::SameLine(0, 5);
            }
            else
            {
                Condi = APIDefs->GetTextureOrCreateFromResource("CONDI_ICON", CONDI, hSelf);
            }
        }
        std::string formattedDamage;
        if(Settings::vsLoggedPlayersOnly){
            formattedDamage = formatDamage(teamData.totalCondiDamageVsPlayers);
        }
        else {
            formattedDamage = formatDamage(teamData.totalCondiDamage);
        }

        if (Settings::showClassNames)
        {
            ImGui::Text("Condi: %s", formattedDamage.c_str());
        }
        else
        {
            ImGui::Text("%s", formattedDamage.c_str());
        }
    }

    // Display specialization bars
    if (Settings::showSpecBars) {
        ImGui::Separator();

        bool sortByDamage = Settings::sortSpecDamage;
        bool vsLogPlayers = Settings::vsLoggedPlayersOnly;
        bool showDamage = Settings::showSpecDamage;

        // Sort specializations by count or damage in descending order
        std::vector<std::pair<std::string, SpecStats>> sortedClasses;

        for (const auto& [eliteSpec, stats] : teamData.eliteSpecStats) {
            sortedClasses.emplace_back(eliteSpec, stats);
        }

        std::sort(sortedClasses.begin(), sortedClasses.end(),
            [sortByDamage](const std::pair<std::string, SpecStats>& a, const std::pair<std::string, SpecStats>& b) {
                if (sortByDamage && !Settings::vsLoggedPlayersOnly) {
                    return a.second.totalDamage > b.second.totalDamage;
                }
                else if (sortByDamage && Settings::vsLoggedPlayersOnly) {
                    return a.second.totalDamageVsPlayers > b.second.totalDamageVsPlayers;
                }
                else {
                    return a.second.count > b.second.count;
                }
            });

        uint64_t maxValue = 0;
        if (!sortedClasses.empty()) {
            if (sortByDamage && !Settings::vsLoggedPlayersOnly) {
                maxValue = sortedClasses[0].second.totalDamage;
            }
            else if (sortByDamage && Settings::vsLoggedPlayersOnly) {
                maxValue = sortedClasses[0].second.totalDamageVsPlayers;
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

            uint64_t totalDamage;
            if(Settings::vsLoggedPlayersOnly)
            {
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


void ratioBarSetup() 
{
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;

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
            ImGui::ColorConvertU32ToFloat4(IM_COL32(0xff, 0x44, 0x44, 0xff)),
            ImGui::ColorConvertU32ToFloat4(IM_COL32(0x33, 0xb5, 0xe5, 0xff)),
            ImGui::ColorConvertU32ToFloat4(IM_COL32(0x99, 0xcc, 0x00, 0xff))
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
            teamCounts[0],
            teamCounts[2],
            teamCounts[1],
            team_colors[0],
            team_colors[2],
            team_colors[1],
            ImVec2(barSize.x, barHeight)
        );
    }
    ImGui::End();
    ImGui::PopStyleVar(4);
}