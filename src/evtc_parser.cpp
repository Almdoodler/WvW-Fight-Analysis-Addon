#define NOMINMAX
#include "evtc_parser.h"
#include "Settings.h"
#include "Shared.h"
#include "utils.h"
#include <zip.h>
#include <thread>
#include <chrono>
#include <filesystem>
#include <shlobj.h>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <cstring>
#include <algorithm>
#include <vector>

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
    APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, ("Agent count: " + std::to_string(agentCount)).c_str());

    std::unordered_map<uint64_t, Agent> agentsByAddress;
    for (uint32_t i = 0; i < agentCount; ++i) {
        Agent agent;
        std::memcpy(&agent.address, bytes.data() + offset, sizeof(uint64_t));
        int32_t professionId, eliteSpecId;
        std::memcpy(&professionId, bytes.data() + offset + 8, sizeof(int32_t));
        std::memcpy(&eliteSpecId, bytes.data() + offset + 12, sizeof(int32_t));
        agent.name = std::string(bytes.data() + offset + 28, 68);
        agent.name.erase(std::find(agent.name.begin(), agent.name.end(), '\0'), agent.name.end());

        if (professions.find(professionId) != professions.end()) {
            agent.profession = professions[professionId];
            if (eliteSpecId != -1 && eliteSpecs.find(eliteSpecId) != eliteSpecs.end()) {
                agent.eliteSpec = eliteSpecs[eliteSpecId];
            }
            else {
                agent.eliteSpec = "Core " + agent.profession;
            }
            agentsByAddress[agent.address] = agent;
        }
        offset += 96;
    }

    // Skip skill data
    uint32_t skillCount;
    std::memcpy(&skillCount, bytes.data() + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t) + (68 * skillCount);

    // Parse combat items
    const size_t eventSize = 64;
    size_t remainingBytes = bytes.size() - offset;
    size_t eventCount = remainingBytes / eventSize;

    std::unordered_map<uint16_t, Agent*> playersBySrcInstid;

    // Initialize times
    uint64_t logStartTime = UINT64_MAX;
    uint64_t logEndTime = 0;
    result.combatStartTime = UINT64_MAX;
    result.combatEndTime = 0;
    uint64_t earliestTime = UINT64_MAX;
    uint64_t latestTime = 0;

    // First pass: Parse events and collect times
    for (size_t i = 0; i < eventCount; ++i) {
        size_t eventOffset = offset + (i * eventSize);
        if (eventOffset + eventSize > bytes.size()) {
            break;
        }

        uint64_t time;
        std::memcpy(&time, bytes.data() + eventOffset, sizeof(uint64_t));

        uint8_t isStateChange;
        std::memcpy(&isStateChange, bytes.data() + eventOffset + 56, sizeof(uint8_t));

        uint64_t srcAgent;
        uint16_t srcInstid;
        std::memcpy(&srcAgent, bytes.data() + eventOffset + 8, sizeof(uint64_t));
        std::memcpy(&srcInstid, bytes.data() + eventOffset + 40, sizeof(uint16_t));

        earliestTime = std::min(earliestTime, time);
        latestTime = std::max(latestTime, time);

        // LogStart & LogEnd events
        if (isStateChange == 9) {  // LogStart
            logStartTime = time;
        }
        else if (isStateChange == 10) {  // LogEnd
            logEndTime = time;
        }

        // EnterCombat & ExitCombat events
        if (isStateChange == 1) {  // EnterCombat
            if (time < result.combatStartTime) {
                result.combatStartTime = time;
            }
        }
        else if (isStateChange == 2) {  // ExitCombat
            if (time > result.combatEndTime) {
                result.combatEndTime = time;
            }
        }
        else if (isStateChange == 0) {  // Normal
            if (agentsByAddress.find(srcAgent) != agentsByAddress.end()) {
                Agent& agent = agentsByAddress[srcAgent];
                agent.id = srcInstid;
                playersBySrcInstid[srcInstid] = &agent;
            }
        }
        else if (isStateChange == 22) {  // Team Change event
            uint32_t teamID;
            std::memcpy(&teamID, bytes.data() + eventOffset + 24, sizeof(uint32_t));
            if (teamID != 0 && agentsByAddress.find(srcAgent) != agentsByAddress.end()) {
                auto it = teamIDs.find(teamID);
                if (it != teamIDs.end()) {
                    Agent& agent = agentsByAddress[srcAgent];
                    agent.team = it->second;
                }
            }
        }
    }

    if (result.combatStartTime == UINT64_MAX) {
        result.combatStartTime = (logStartTime != UINT64_MAX) ? logStartTime : earliestTime;
    }
    if (result.combatEndTime == 0) {
        result.combatEndTime = (logEndTime != 0) ? logEndTime : latestTime;
    }

    // Second pass: Count deaths and downs
    for (size_t i = 0; i < eventCount; ++i) {
        size_t eventOffset = offset + (i * eventSize);
        if (eventOffset + eventSize > bytes.size()) {
            break;
        }

        uint8_t isStateChange;
        std::memcpy(&isStateChange, bytes.data() + eventOffset + 56, sizeof(uint8_t));

        if (isStateChange == 4 || isStateChange == 5) {  // ChangeDead or ChangeDown event
            uint16_t srcInstid;
            std::memcpy(&srcInstid, bytes.data() + eventOffset + 40, sizeof(uint16_t));

            if (playersBySrcInstid.find(srcInstid) != playersBySrcInstid.end()) {
                Agent* player = playersBySrcInstid[srcInstid];
                const std::string& team = player->team;
                if (team != "Unknown") {
                    if (isStateChange == 4) {
                        result.teamStats[team].totalDeaths++;
                    }
                    else {
                        result.teamStats[team].totalDowned++;
                    }
                }
            }
        }
    }

    // Third pass: Process damage events
// Third pass: Process damage events
    for (size_t i = 0; i < eventCount; ++i) {
        size_t eventOffset = offset + (i * eventSize);
        if (eventOffset + eventSize > bytes.size()) {
            break;
        }

        uint8_t isStateChange;
        std::memcpy(&isStateChange, bytes.data() + eventOffset + 56, sizeof(uint8_t));

        // Normal events
        if (isStateChange == 0) {
            uint8_t isActivation;
            uint8_t isBuff;
            uint8_t resultCode;
            std::memcpy(&isActivation, bytes.data() + eventOffset + 57, sizeof(uint8_t));
            std::memcpy(&isBuff, bytes.data() + eventOffset + 58, sizeof(uint8_t));
            std::memcpy(&resultCode, bytes.data() + eventOffset + 59, sizeof(uint8_t));

            // Damage events
            if (isActivation == 0) {
                if (resultCode == 1 || resultCode == 2 || resultCode == 3) {  // Normal, Critical, Glance
                    uint16_t srcInstid;
                    uint16_t dstInstid;
                    std::memcpy(&srcInstid, bytes.data() + eventOffset + 40, sizeof(uint16_t));
                    std::memcpy(&dstInstid, bytes.data() + eventOffset + 42, sizeof(uint16_t));

                    int32_t damageValue;
                    std::memcpy(&damageValue, bytes.data() + eventOffset + 16, sizeof(int32_t));

                    if (damageValue != 0) {
                        auto srcIt = playersBySrcInstid.find(srcInstid);
                        auto dstIt = playersBySrcInstid.find(dstInstid);
                        if (srcIt != playersBySrcInstid.end() && dstIt != playersBySrcInstid.end()) {
                            // Both source and destination are players
                            Agent* agent = srcIt->second;
                            const std::string& team = agent->team;
                            if (team != "Unknown") {
                                result.teamStats[team].totalDamage += damageValue;
                            }
                        }
                    }
                }
            }
        }
    }


    // Collect statistics
    for (const auto& [srcInstid, player] : playersBySrcInstid) {
        if (player->team != "Unknown") {
            result.teamStats[player->team].totalPlayers++;
            result.teamStats[player->team].eliteSpecCounts[player->eliteSpec]++;
        }
        else {
            APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
                ("Player with unknown team - srcInstid: " + std::to_string(srcInstid) +
                    ", Name: " + player->name).c_str());
        }
    }

    // Log the totals
    for (const auto& [teamName, stats] : result.teamStats) {
        std::string message = "Team: " + teamName +
            ", Total Damage: " + std::to_string(stats.totalDamage) +
            ", Total Players: " + std::to_string(stats.totalPlayers) +
            ", Total Downs: " + std::to_string(stats.totalDowned) +
            ", Total Deaths: " + std::to_string(stats.totalDeaths);
        APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, message.c_str());
    }

    APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
        ("Parsed EVTC file. Found " + std::to_string(playersBySrcInstid.size()) + " unique players").c_str());

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