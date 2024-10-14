#include "utils.h"
#include "Shared.h"
#include "Settings.h"
#include "evtc_parser.h"
#include <thread>
#include <chrono>
#include "utils.h"
#include <zip.h>
#include <shlobj.h>
#include <sstream>
#include <filesystem>

const char* ADDON_WINDOW_VISIBLE = "KB_MISTINSIGHT_WINDOW_VISIBLE";

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

std::string formatDamage(float damage) {
    if (damage >= 1000000) {
        if (fmod(damage, 1000000.0f) == 0.0f) {
            return std::to_string(static_cast<int>(damage / 1000000.0f)) + "M";  // No decimal for whole numbers in millions
        }
        else {
            char buffer[10];
            snprintf(buffer, sizeof(buffer), "%.1fM", damage / 1000000.0f);  // Show decimal for non-whole numbers in millions
            return std::string(buffer);
        }
    }
    else if (damage >= 1000) {
        if (fmod(damage, 1000.0f) == 0.0f) {
            return std::to_string(static_cast<int>(damage / 1000.0f)) + "k";  // No decimal for whole numbers in thousands
        }
        else {
            char buffer[10];
            snprintf(buffer, sizeof(buffer), "%.1fk", damage / 1000.0f);  // Show decimal for non-whole numbers in thousands
            return std::string(buffer);
        }
    }
    else {
        return std::to_string(static_cast<int>(damage));  // Show raw value for less than 1000
    }
}

void monitorDirectory()
{
    try
    {
        std::filesystem::path dirPath;

        // Check if the custom log directory is set and valid
        if (!Settings::LogDirectoryPath.empty())
        {
            dirPath = Settings::LogDirectoryPath;

            // Convert to filesystem path to handle any path issues
            dirPath = std::filesystem::path(dirPath);

            // Validate the directory
            if (!std::filesystem::exists(dirPath) || !std::filesystem::is_directory(dirPath))
            {
                APIDefs->Log(ELogLevel_WARNING, ADDON_NAME, ("Specified log directory does not exist: " + dirPath.string()).c_str());
                dirPath.clear(); // Fall back to default path
            }
        }

        if (dirPath.empty())
        {
            // Use default path
            char documentsPath[MAX_PATH];
            if (FAILED(SHGetFolderPath(nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, documentsPath)))
            {
                APIDefs->Log(ELogLevel_WARNING, ADDON_NAME, "Failed to get path to user's Documents folder.");
                return;
            }

            dirPath = std::filesystem::path(documentsPath) / "Guild Wars 2" / "addons" / "arcdps" / "arcdps.cbtlogs" / "WvW (1)";
        }

        // Now dirPath is the directory we will monitor

        HANDLE hDir = CreateFileW(
            dirPath.wstring().c_str(),
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
            nullptr);

        if (hDir == INVALID_HANDLE_VALUE)
        {
            APIDefs->Log(ELogLevel_WARNING, ADDON_NAME, ("Failed to open directory for monitoring: " + dirPath.string()).c_str());
            return;
        }

        char buffer[4096];
        OVERLAPPED overlapped = { 0 };
        HANDLE hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
        overlapped.hEvent = hEvent;

        while (!stopMonitoring)
        {
            memset(buffer, 0, sizeof(buffer));
            ResetEvent(hEvent);

            BOOL success = ReadDirectoryChangesW(
                hDir,
                buffer,
                sizeof(buffer),
                FALSE,
                FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION,
                nullptr,
                &overlapped,
                nullptr);

            if (!success)
            {
                APIDefs->Log(ELogLevel_WARNING, ADDON_NAME, "Failed to read directory changes.");
                break;
            }

            DWORD waitStatus = WaitForSingleObject(hEvent, 500); // Wait with timeout

            if (stopMonitoring)
            {
                CancelIoEx(hDir, &overlapped); // Cancel pending I/O
                break;
            }

            if (waitStatus == WAIT_OBJECT_0)
            {
                DWORD bytesTransferred = 0;
                if (GetOverlappedResult(hDir, &overlapped, &bytesTransferred, FALSE))
                {
                    FILE_NOTIFY_INFORMATION* fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer);
                    do
                    {
                        std::wstring filenameW(fni->FileName, fni->FileNameLength / sizeof(WCHAR));

                        if ((fni->Action == FILE_ACTION_ADDED ||
                            fni->Action == FILE_ACTION_MODIFIED ||
                            fni->Action == FILE_ACTION_RENAMED_NEW_NAME) &&
                            filenameW.size() >= 6 && filenameW.compare(filenameW.size() - 6, 6, L".zevtc") == 0)
                        {
                            std::filesystem::path fullPath = dirPath / filenameW;

                            waitForFile(fullPath.string());

                            processNewEVTCFile(fullPath.string());
                        }

                        if (fni->NextEntryOffset == 0)
                            break;
                        fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(reinterpret_cast<BYTE*>(fni) + fni->NextEntryOffset);
                    } while (true);
                }
                else
                {
                    APIDefs->Log(ELogLevel_WARNING, ADDON_NAME, "Failed to get overlapped result.");
                }
            }
            else if (waitStatus == WAIT_TIMEOUT)
            {
                continue;
            }
            else
            {
                APIDefs->Log(ELogLevel_WARNING, ADDON_NAME, "Wait error.");
                break;
            }
        }

        CloseHandle(hEvent);
        CloseHandle(hDir);
    }
    catch (const std::exception& ex)
    {
        APIDefs->Log(ELogLevel_WARNING, ADDON_NAME, ("Exception in directory monitoring thread: " + std::string(ex.what())).c_str());
    }
}



void waitForFile(const std::string& filePath)
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD previousSize = 0;

    while (true)
    {
        hFile = CreateFile(
            filePath.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,
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
        APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, ("Failed to open zip file. Error code: " + std::to_string(err)).c_str());
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