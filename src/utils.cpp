#include "utils.h"
#include "resource.h"
#include "Shared.h"
#include "Settings.h"
#include "evtc_parser.h"
#include "utils.h"
#include <thread>
#include <chrono>
#include "utils.h"
#include <zip.h>
#include <shlobj.h>
#include <sstream>
#include <filesystem>
#include <cmath>
#include <string>
#include <cstdio>
#include <codecvt>
#include <iostream>
#include <locale>
#include <string>
#include <Windows.h>

const char* ADDON_WINDOW_VISIBLE = "KB_MISTINSIGHT_WINDOW_VISIBLE";

static const wchar_t CP1252_UNICODE_TABLE[] =
L"\u0000\u0001\u0002\u0003\u0004\u0005\u0006\u0007"
L"\u0008\u0009\u000A\u000B\u000C\u000D\u000E\u000F"
L"\u0010\u0011\u0012\u0013\u0014\u0015\u0016\u0017"
L"\u0018\u0019\u001A\u001B\u001C\u001D\u001E\u001F"
L"\u0020\u0021\u0022\u0023\u0024\u0025\u0026\u0027"
L"\u0028\u0029\u002A\u002B\u002C\u002D\u002E\u002F"
L"\u0030\u0031\u0032\u0033\u0034\u0035\u0036\u0037"
L"\u0038\u0039\u003A\u003B\u003C\u003D\u003E\u003F"
L"\u0040\u0041\u0042\u0043\u0044\u0045\u0046\u0047"
L"\u0048\u0049\u004A\u004B\u004C\u004D\u004E\u004F"
L"\u0050\u0051\u0052\u0053\u0054\u0055\u0056\u0057"
L"\u0058\u0059\u005A\u005B\u005C\u005D\u005E\u005F"
L"\u0060\u0061\u0062\u0063\u0064\u0065\u0066\u0067"
L"\u0068\u0069\u006A\u006B\u006C\u006D\u006E\u006F"
L"\u0070\u0071\u0072\u0073\u0074\u0075\u0076\u0077"
L"\u0078\u0079\u007A\u007B\u007C\u007D\u007E\u007F"
L"\u20AC\u0020\u201A\u0192\u201E\u2026\u2020\u2021"
L"\u02C6\u2030\u0160\u2039\u0152\u0020\u017D\u0020"
L"\u0020\u2018\u2019\u201C\u201D\u2022\u2013\u2014"
L"\u02DC\u2122\u0161\u203A\u0153\u0020\u017E\u0178"
L"\u00A0\u00A1\u00A2\u00A3\u00A4\u00A5\u00A6\u00A7"
L"\u00A8\u00A9\u00AA\u00AB\u00AC\u00AD\u00AE\u00AF"
L"\u00B0\u00B1\u00B2\u00B3\u00B4\u00B5\u00B6\u00B7"
L"\u00B8\u00B9\u00BA\u00BB\u00BC\u00BD\u00BE\u00BF"
L"\u00C0\u00C1\u00C2\u00C3\u00C4\u00C5\u00C6\u00C7"
L"\u00C8\u00C9\u00CA\u00CB\u00CC\u00CD\u00CE\u00CF"
L"\u00D0\u00D1\u00D2\u00D3\u00D4\u00D5\u00D6\u00D7"
L"\u00D8\u00D9\u00DA\u00DB\u00DC\u00DD\u00DE\u00DF"
L"\u00E0\u00E1\u00E2\u00E3\u00E4\u00E5\u00E6\u00E7"
L"\u00E8\u00E9\u00EA\u00EB\u00EC\u00ED\u00EE\u00EF"
L"\u00F0\u00F1\u00F2\u00F3\u00F4\u00F5\u00F6\u00F7"
L"\u00F8\u00F9\u00FA\u00FB\u00FC\u00FD\u00FE\u00FF";

void initMaps() {
    // Map of profession IDs to profession names
    professions = {
        {1, "Guardian"},
        {2, "Warrior"},
        {3, "Engineer"},
        {4, "Ranger"},
        {5, "Thief"},
        {6, "Elementalist"},
        {7, "Mesmer"},
        {8, "Necromancer"},
        {9, "Revenant"}
    };

    // Map of elite specialization IDs to elite specialization names
    eliteSpecs = {
        {5,  "Druid"},
        {7,  "Daredevil"},
        {18, "Berserker"},
        {27, "Dragonhunter"},
        {34, "Reaper"},
        {40, "Chronomancer"},
        {43, "Scrapper"},
        {48, "Tempest"},
        {52, "Herald"},
        {55, "Soulbeast"},
        {56, "Weaver"},
        {57, "Holosmith"},
        {58, "Deadeye"},
        {59, "Mirage"},
        {60, "Scourge"},
        {61, "Spellbreaker"},
        {62, "Firebrand"},
        {63, "Renegade"},
        {64, "Harbinger"},
        {65, "Willbender"},
        {66, "Virtuoso"},
        {67, "Catalyst"},
        {68, "Bladesworn"},
        {69, "Vindicator"},
        {70, "Mechanist"},
        {71, "Specter"},
        {72, "Untamed"}
    };

    // Map of elite specialization short names to full names
    eliteSpecShortNames = {
    {"Core Guardian", "Gdn"},
    {"Dragonhunter", "Dgh"},
    {"Firebrand", "Fbd"},
    {"Willbender", "Wbd"},
    {"Core Warrior", "War"},
    {"Berserker", "Brs"},
    {"Spellbreaker", "Spb"},
    {"Bladesworn", "Bds"},
    {"Core Engineer", "Eng"},
    {"Scrapper", "Scr"},
    {"Holosmith", "Hls"},
    {"Mechanist", "Mec"},
    {"Core Ranger", "Rgr"},
    {"Druid", "Dru"},
    {"Soulbeast", "Slb"},
    {"Untamed", "Unt"},
    {"Core Thief", "Thf"},
    {"Daredevil", "Dar"},
    {"Deadeye", "Ded"},
    {"Specter", "Spe"},
    {"Core Elementalist", "Ele"},
    {"Tempest", "Tmp"},
    {"Weaver", "Wea"},
    {"Catalyst", "Cat"},
    {"Core Mesmer", "Mes"},
    {"Chronomancer", "Chr"},
    {"Mirage", "Mir"},
    {"Virtuoso", "Vir"},
    {"Core Necromancer", "Nec"},
    {"Reaper", "Rea"},
    {"Scourge", "Scg"},
    {"Harbinger", "Har"},
    {"Core Revenant", "Rev"},
    {"Herald", "Her"},
    {"Renegade", "Ren"},
    {"Vindicator", "Vin"}
    };


    // Map of team IDs to team names
    teamIDs = {
        {697,  "Red"}, {705,  "Red"}, {706,  "Red"}, {882,  "Red"}, {2520, "Red"},
        {39,   "Green"}, {2739, "Green"}, {2741, "Green"}, {2752, "Green"}, {2763, "Green"},
        {432,  "Blue"}, {1277, "Blue"}, {1989, "Blue"}
    };

    // Map of elite specializations to professions
    eliteSpecToProfession = {
        {"Dragonhunter",    "Guardian"},
        {"Firebrand",       "Guardian"},
        {"Willbender",      "Guardian"},
        {"Core Guardian",   "Guardian"},
        {"Berserker",       "Warrior"},
        {"Spellbreaker",    "Warrior"},
        {"Bladesworn",      "Warrior"},
        {"Core Warrior",    "Warrior"},
        {"Scrapper",        "Engineer"},
        {"Holosmith",       "Engineer"},
        {"Mechanist",       "Engineer"},
        {"Core Engineer",   "Engineer"},
        {"Druid",           "Ranger"},
        {"Soulbeast",       "Ranger"},
        {"Untamed",         "Ranger"},
        {"Core Ranger",     "Ranger"},
        {"Daredevil",       "Thief"},
        {"Deadeye",         "Thief"},
        {"Specter",         "Thief"},
        {"Core Thief",      "Thief"},
        {"Tempest",         "Elementalist"},
        {"Weaver",          "Elementalist"},
        {"Catalyst",        "Elementalist"},
        {"Core Elementalist", "Elementalist"},
        {"Chronomancer",    "Mesmer"},
        {"Mirage",          "Mesmer"},
        {"Virtuoso",        "Mesmer"},
        {"Core Mesmer",     "Mesmer"},
        {"Reaper",          "Necromancer"},
        {"Scourge",         "Necromancer"},
        {"Harbinger",       "Necromancer"},
        {"Core Necromancer","Necromancer"},
        {"Herald",          "Revenant"},
        {"Renegade",        "Revenant"},
        {"Vindicator",      "Revenant"},
        {"Core Revenant",   "Revenant"}
    };

    // Map of professions to colors (ImVec4)
    professionColors = {
        {"Guardian",     ImVec4(10 / 255.0f, 222 / 255.0f, 255 / 255.0f, 110 / 255.0f)},
        {"Warrior",      ImVec4(255 / 255.0f, 212 / 255.0f, 61 / 255.0f, 110 / 255.0f)},
        {"Engineer",     ImVec4(227 / 255.0f, 115 / 255.0f, 41 / 255.0f, 110 / 255.0f)},
        {"Ranger",       ImVec4(135 / 255.0f, 222 / 255.0f, 10 / 255.0f, 110 / 255.0f)},
        {"Thief",        ImVec4(227 / 255.0f, 94 / 255.0f, 115 / 255.0f, 115 / 255.0f)},
        {"Elementalist", ImVec4(247 / 255.0f, 56 / 255.0f, 56 / 255.0f, 110 / 255.0f)},
        {"Mesmer",       ImVec4(204 / 255.0f, 59 / 255.0f, 209 / 255.0f, 110 / 255.0f)},
        {"Necromancer",  ImVec4(5 / 255.0f, 227 / 255.0f, 125 / 255.0f, 110 / 255.0f)},
        {"Revenant",     ImVec4(161 / 255.0f, 41 / 255.0f, 41 / 255.0f, 115 / 255.0f)}
    };
}

std::unordered_map<std::string, TextureInfo> textureMap = {
    {"Berserker", {BERSERKER, &Berserker}},
    {"Bladesworn", {BLADESWORN, &Bladesworn}},
    {"Catalyst", {CATALYST, &Catalyst}},
    {"Chronomancer", {CHRONOMANCER, &Chronomancer}},
    {"Daredevil", {DAREDEVIL, &Daredevil}},
    {"Deadeye", {DEADEYE, &Deadeye}},
    {"Dragonhunter", {DRAGONHUNTER, &Dragonhunter}},
    {"Druid", {DRUID, &Druid}},
    {"Core Elementalist", {ELEMENTALIST, &Elementalist}},
    {"Core Engineer", {ENGINEER, &Engineer}},
    {"Firebrand", {FIREBRAND, &Firebrand}},
    {"Core Guardian", {GUARDIAN, &Guardian}},
    {"Harbinger", {HARBINGER, &Harbinger}},
    {"Herald", {HERALD, &Herald}},
    {"Holosmith", {HOLOSMITH, &Holosmith}},
    {"Mechanist", {MECHANIST, &Mechanist}},
    {"Core Mesmer", {MESMER, &Mesmer}},
    {"Mirage", {MIRAGE, &Mirage}},
    {"Core Necromancer", {NECROMANCER, &Necromancer}},
    {"Core Ranger", {RANGER, &Ranger}},
    {"Reaper", {REAPER, &Reaper}},
    {"Renegade", {RENEGADE, &Renegade}},
    {"Core Revenant", {REVENANT, &Revenant}},
    {"Scrapper", {SCRAPPER, &Scrapper}},
    {"Scourge", {SCOURGE, &Scourge}},
    {"Soulbeast", {SOULBEAST, &Soulbeast}},
    {"Specter", {SPECTER, &Specter}},
    {"Spellbreaker", {SPELLBREAKER, &Spellbreaker}},
    {"Tempest", {TEMPEST, &Tempest}},
    {"Core Thief", {THIEF, &Thief}},
    {"Untamed", {UNTAMED, &Untamed}},
    {"Vindicator", {VINDICATOR, &Vindicator}},
    {"Virtuoso", {VIRTUOSO, &Virtuoso}},
    {"Core Warrior", {WARRIOR, &Warrior}},
    {"Weaver", {WEAVER, &Weaver}},
    {"Willbender", {WILLBENDER, &Willbender}}
};

ImVec4 GetTeamColor(const std::string& teamName)
{
    if (teamName == "Red")
        return ImGui::ColorConvertU32ToFloat4(IM_COL32(0xff, 0x44, 0x44, 0xff)); // Red
    else if (teamName == "Blue")
        return ImGui::ColorConvertU32ToFloat4(IM_COL32(0x33, 0xb5, 0xe5, 0xff)); // Blue
    else if (teamName == "Green")
        return ImGui::ColorConvertU32ToFloat4(IM_COL32(0x99, 0xcc, 0x00, 0xff)); // Green
    else
        return ImGui::GetStyleColorVec4(ImGuiCol_Text); // Default text color
}

std::string generateLogDisplayName(const std::string& filename, uint64_t combatStartMs, uint64_t combatEndMs) 
{

    size_t lastDotPosition = filename.find_last_of('.');
    std::string filenameWithoutExt = (lastDotPosition != std::string::npos)
        ? filename.substr(0, lastDotPosition)
        : filename;

    uint64_t durationMs = (combatEndMs >= combatStartMs) ? (combatEndMs - combatStartMs) : 0;
    std::chrono::milliseconds duration(durationMs);
    int minutes = std::chrono::duration_cast<std::chrono::minutes>(duration).count();
    int seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count() % 60;

    std::string displayName = filenameWithoutExt + " ("
        + std::to_string(minutes) + "m "
        + std::to_string(seconds) + "s)";

    return displayName;
}

std::string formatDuration(uint64_t milliseconds) {
    std::chrono::milliseconds duration(milliseconds);
    int minutes = std::chrono::duration_cast<std::chrono::minutes>(duration).count();
    int seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count() % 60;

    return std::to_string(minutes) + "m " + std::to_string(seconds) + "s";
}

bool isRunningUnderWine()
{
    if (Settings::forceLinuxCompatibilityMode) {
        return true;
    }
    HKEY hKey;
    LONG result = RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Wine", 0, KEY_READ, &hKey);
    if (result == ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return true;
    }
    else
    {
        result = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\Wine", 0, KEY_READ, &hKey);
        if (result == ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return true;
        }
    }

    HMODULE hNtDll = GetModuleHandleA("ntdll.dll");
    if (hNtDll)
    {
        FARPROC wine_get_version = GetProcAddress(hNtDll, "wine_get_version");
        if (wine_get_version)
        {
            return true;
        }
    }

    return false;
}

Texture** getTextureInfo(const std::string& eliteSpec, int* outResourceId) {
    auto it = textureMap.find(eliteSpec);
    if (it != textureMap.end()) {
        *outResourceId = it->second.resourceId;
        return it->second.texture;
    }
    else {
        *outResourceId = 0;
        return nullptr;
    }
}

std::string formatDamage(double damage) {
    if (damage >= 1'000'000.0) {
        if (std::fmod(damage, 1'000'000.0) == 0.0) {
            return std::to_string(static_cast<int>(damage / 1'000'000.0)) + "M";
        }
        else {
            char buffer[10];
            std::snprintf(buffer, sizeof(buffer), "%.1fM", damage / 1'000'000.0);
            if (buffer[strlen(buffer) - 3] == '.' && buffer[strlen(buffer) - 2] == '0') {
                buffer[strlen(buffer) - 3] = 'M';  // Overwrite '.' with 'M'
                buffer[strlen(buffer) - 2] = '\0'; // Terminate string after 'M'
            }
            return std::string(buffer);
        }
    }
    else if (damage >= 1'000.0) {
        if (std::fmod(damage, 1'000.0) == 0.0) {
            return std::to_string(static_cast<int>(damage / 1'000.0)) + "k";
        }
        else {
            char buffer[10];
            std::snprintf(buffer, sizeof(buffer), "%.1fk", damage / 1'000.0);
            if (buffer[strlen(buffer) - 3] == '.' && buffer[strlen(buffer) - 2] == '0') {
                buffer[strlen(buffer) - 3] = 'k';  // Overwrite '.' with 'k'
                buffer[strlen(buffer) - 2] = '\0'; // Terminate string after 'k'
            }
            return std::string(buffer);
        }
    }
    else {
        return std::to_string(static_cast<int>(damage));
    }
}


void waitForFile(const std::string& filePath)
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD previousSize = 0;
    //APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
    //    ("Waiting for file: " + filePath).c_str());
    while (true)
    {
        hFile = CreateFile(
            filePath.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);

        if (hFile == INVALID_HANDLE_VALUE)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        DWORD currentSize = GetFileSize(hFile, nullptr);

        if (currentSize == previousSize && currentSize != INVALID_FILE_SIZE)
        {
            CloseHandle(hFile);
            break;
        }

        previousSize = currentSize;
        CloseHandle(hFile);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

std::vector<char> extractZipFile(const std::string& filePath) {
    int err = 0;
    zip* z = zip_open(filePath.c_str(), 0, &err);
    if (!z) {
        APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, ("Failed to open zip file. Error code: " + std::to_string(err) + " filepath: " + filePath).c_str());
        return std::vector<char>();
    }

    zip_stat_t zstat;
    zip_stat_init(&zstat);
    zip_stat_index(z, 0, 0, &zstat);

    std::vector<char> buffer(zstat.size);
    zip_file* f = zip_fopen_index(z, 0, 0);
    if (!f) {
        zip_close(z);
        APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, "Failed to open file in zip");
        return std::vector<char>();
    }

    zip_fread(f, buffer.data(), zstat.size);
    zip_fclose(f);
    zip_close(z);

    return buffer;
}

std::filesystem::path getArcPath()
{
    std::filesystem::path filename = APIDefs->Paths.GetAddonDirectory("arcdps\\arcdps.ini");

    char buffer[256] = { 0 };

    GetPrivateProfileStringA(
        "session",
        "boss_encounter_path",
        "",
        buffer,
        sizeof(buffer),
        filename.string().c_str()
    );

    std::filesystem::path boss_encounter_path = buffer;

    return boss_encounter_path;
}

// Convert a
// * Windows-1252 (aka CP1252) stored as an std::string
// into an
// * UTF8 std::string.
std::string CP1252_to_UTF8(const std::string& byte_array) {
    // Byte array => Unicode.
    std::wstring unicode(byte_array.size(), L' ');
    for (size_t i = 0; i < unicode.size(); ++i)
        unicode[i] = CP1252_UNICODE_TABLE[(uint8_t)byte_array[i]];

    // Unicode => UTF8.
    int utf8_size = WideCharToMultiByte(CP_UTF8, 0, unicode.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (utf8_size == 0) {
        // Fehlerbehandlung
        return "";
    }

    std::string utf8_string(utf8_size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, unicode.c_str(), -1, &utf8_string[0], utf8_size, nullptr, nullptr);

    // Entfernen des Nullterminators
    utf8_string.resize(utf8_size - 1);

    return utf8_string;
}