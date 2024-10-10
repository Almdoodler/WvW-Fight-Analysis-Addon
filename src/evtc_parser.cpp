#include "evtc_parser.h"
#include "Settings.h"
#include "Shared.h"
#include "utils.h"
#include <zip.h>
#include <algorithm>
#include <thread>
#include <chrono>
#include <filesystem>
#include <shlobj.h>
#include <sstream>


ParsedData parseEVTCFile(const std::string& filePath) {
    ParsedData result;
    std::vector<char> bytes = extractZipFile(filePath);
    if (bytes.empty()) {
        APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, "Failed to extract EVTC file");
        return result;
    }

    // Parse header (16 bytes)
    size_t offset = 16;

    // Parse agents
    uint32_t agentCount;
    std::memcpy(&agentCount, bytes.data() + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    std::unordered_map<uint64_t, Agent> players;
    for (uint32_t i = 0; i < agentCount; ++i) {
        Agent agent;
        std::memcpy(&agent.address, bytes.data() + offset, sizeof(uint64_t));
        int32_t professionId, eliteSpecId;
        std::memcpy(&professionId, bytes.data() + offset + 8, sizeof(int32_t));
        std::memcpy(&eliteSpecId, bytes.data() + offset + 12, sizeof(int32_t));
        agent.name = std::string(bytes.data() + offset + 28, 68);
        agent.name.erase(std::find(agent.name.begin(), agent.name.end(), '\0'), agent.name.end());
        agent.team = "Unknown";
        if (professions.find(professionId) != professions.end()) {
            agent.profession = professions[professionId];
            if (eliteSpecId != -1 && eliteSpecs.find(eliteSpecId) != eliteSpecs.end()) {
                agent.eliteSpec = eliteSpecs[eliteSpecId];
            }
            else {
                agent.eliteSpec = "Core " + agent.profession;
            }
            players[agent.address] = agent;
        }
        offset += 96;
    }

    // Skip skill data
    uint32_t skillCount;
    std::memcpy(&skillCount, bytes.data() + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t) + (68 * skillCount);

    // First pass: Assign teams to players
    const size_t eventSize = 64;
    size_t remainingBytes = bytes.size() - offset;
    size_t eventCount = remainingBytes / eventSize;
    for (size_t i = 0; i < eventCount; ++i) {
        size_t eventOffset = offset + (i * eventSize);
        if (eventOffset + eventSize > bytes.size()) {
            break;
        }
        int isStateChange;
        std::memcpy(&isStateChange, bytes.data() + eventOffset + 56, sizeof(int));

        if (isStateChange == 22) {  // Team Change event
            uint64_t srcAgent;
            uint32_t teamID;
            std::memcpy(&srcAgent, bytes.data() + eventOffset + 8, sizeof(uint64_t));
            std::memcpy(&teamID, bytes.data() + eventOffset + 24, sizeof(uint32_t));
            if (teamID != 0 && players.find(srcAgent) != players.end()) {
                players[srcAgent].team = teamIDs[teamID];
                APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
                    ("Player assigned to team: " + players[srcAgent].name + " -> " + players[srcAgent].team).c_str());
            }
        }
    }

    // Second pass: Count deaths
    for (size_t i = 0; i < eventCount; ++i) {
        size_t eventOffset = offset + (i * eventSize);
        if (eventOffset + eventSize > bytes.size()) {
            break;
        }
        int isStateChange;
        std::memcpy(&isStateChange, bytes.data() + eventOffset + 56, sizeof(int));

        if (isStateChange == 4) {  // ChangeDead event
            uint64_t srcAgent;
            std::memcpy(&srcAgent, bytes.data() + eventOffset + 8, sizeof(uint64_t));
            if (players.find(srcAgent) != players.end()) {
                const std::string& team = players[srcAgent].team;
                if (team != "Unknown") {
                    result.teamStats[team].totalDeaths++;
                    APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
                        ("Death recorded for team: " + team + ", Player: " + players[srcAgent].name +
                            ", Total deaths: " + std::to_string(result.teamStats[team].totalDeaths)).c_str());
                }
            }
        }
        else if (isStateChange == 5) {
            uint64_t srcAgent;
            std::memcpy(&srcAgent, bytes.data() + eventOffset + 8, sizeof(uint64_t));
            if (players.find(srcAgent) != players.end()) {
                const std::string& team = players[srcAgent].team;
                if (team != "Unknown") {
                    result.teamStats[team].totalDowned++;
                    APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
                        ("Downed player recorded for team: " + team + ", Player: " + players[srcAgent].name +
                            ", Total downed: " + std::to_string(result.teamStats[team].totalDeaths)).c_str());
                }
            }
        }
    }

    // Collect statistics
    for (const auto& playerPair : players) {
        const Agent& player = playerPair.second;
        if (player.team != "Unknown") {
            result.teamStats[player.team].totalPlayers++;
            result.teamStats[player.team].eliteSpecCounts[player.eliteSpec]++;
        }
    }

    APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, ("Parsed EVTC file. Found " + std::to_string(players.size()) + " players").c_str());

    // Log total deaths for each team
    for (const auto& teamPair : result.teamStats) {
        APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
            ("Team: " + teamPair.first + ", Total players: " + std::to_string(teamPair.second.totalPlayers) +
                ", Total deaths: " + std::to_string(teamPair.second.totalDeaths)).c_str());
    }

    return result;
}

void parseInitialLogs() {
    try {
        std::filesystem::path dirPath;

        // Check if the custom log directory is set and valid
        if (!Settings::LogDirectoryPath.empty()) {
            dirPath = Settings::LogDirectoryPath;

            // Convert backslashes to the correct format if necessary
            dirPath = std::filesystem::path(dirPath);

            if (!std::filesystem::exists(dirPath) || !std::filesystem::is_directory(dirPath)) {
                APIDefs->Log(ELogLevel_WARNING, ADDON_NAME, ("Specified log directory does not exist: " + dirPath.string()).c_str());
                dirPath.clear(); // Fall back to default path below
            }
        }

        if (dirPath.empty()) {
            // Use default path
            char documentsPath[MAX_PATH];
            if (FAILED(SHGetFolderPath(nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, documentsPath))) {
                APIDefs->Log(ELogLevel_WARNING, ADDON_NAME, "Failed to get path to user's Documents folder.");
                return;
            }

            dirPath = std::filesystem::path(documentsPath) / "Guild Wars 2" / "addons" / "arcdps" / "arcdps.cbtlogs" / "WvW (1)";
        }

        // Now dirPath is the directory we will use
        // Parse the latest 10 logs
        std::vector<std::pair<std::string, FILETIME>> files;

        std::string searchPattern = (dirPath / "*.zevtc").string();

        WIN32_FIND_DATA findFileData;
        HANDLE hFind = FindFirstFile(searchPattern.c_str(), &findFileData);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    std::string filename = findFileData.cFileName;
                    FILETIME ftWrite = findFileData.ftLastWriteTime;

                    files.push_back(std::make_pair(filename, ftWrite));
                }
            } while (FindNextFile(hFind, &findFileData) != 0);
            FindClose(hFind);
        }
        else {
            APIDefs->Log(ELogLevel_WARNING, ADDON_NAME, ("No .zevtc files found in directory: " + dirPath.string()).c_str());
        }

        // Sort the files by modification time, newest first
        std::sort(files.begin(), files.end(), [](const std::pair<std::string, FILETIME>& a, const std::pair<std::string, FILETIME>& b) {
            return CompareFileTime(&a.second, &b.second) > 0; // a is newer than b
            });

        // Take the first 10 files
        size_t numFilesToParse = std::min<size_t>(files.size(), 10);

        for (size_t i = 0; i < numFilesToParse; ++i) {
            const std::string& filename = files[i].first;
            std::filesystem::path fullPath = dirPath / filename;

            // Wait for the file to be fully written
            waitForFile(fullPath.string());

            // Parse the file
            ParsedLog log;
            log.filename = filename;
            log.data = parseEVTCFile(fullPath.string());

            {
                std::lock_guard<std::mutex> lock(parsedLogsMutex);
                parsedLogs.push_back(log);

                while (parsedLogs.size() > 10) {
                    parsedLogs.pop_back();
                }
                currentLogIndex = 0;
            }
        }

        // Indicate that initial parsing is complete
        initialParsingComplete = true;
    }
    catch (const std::exception& ex) {
        // Log the exception
        APIDefs->Log(ELogLevel_WARNING, ADDON_NAME, ("Exception in initial parsing thread: " + std::string(ex.what())).c_str());
    }
}


void processNewEVTCFile(const std::string& filePath)
{
    std::filesystem::path path(filePath);
    std::string filename = path.filename().string();

    ParsedLog log;
    log.filename = filename;
    log.data = parseEVTCFile(filePath);

    {
        std::lock_guard<std::mutex> lock(parsedLogsMutex);
        parsedLogs.push_front(log);

        while (parsedLogs.size() > 10)
        {
            parsedLogs.pop_back();
        }

        currentLogIndex = 0;  // The newest log is always at index 0
    }
}