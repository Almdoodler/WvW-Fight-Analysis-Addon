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


void parseAgents(const std::vector<char>& bytes, size_t& offset, uint32_t agentCount,
    std::unordered_map<uint64_t, Agent>& agentsByAddress) {
    APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, ("Agent count: " + std::to_string(agentCount)).c_str());

    for (uint32_t i = 0; i < agentCount; ++i) {
        Agent agent;
        std::memcpy(&agent.address, bytes.data() + offset, sizeof(uint64_t));
        std::memcpy(&agent.professionId, bytes.data() + offset + 8, sizeof(uint32_t));
        std::memcpy(&agent.eliteSpecId, bytes.data() + offset + 12, sizeof(int32_t));

        agent.name = std::string(bytes.data() + offset + 28, 68);
        agent.name.erase(std::find(agent.name.begin(), agent.name.end(), '\0'), agent.name.end());

        // Map profession and elite specialization
        if (professions.find(agent.professionId) != professions.end()) {
            agent.profession = professions[agent.professionId];
            if (agent.eliteSpecId != -1 && eliteSpecs.find(agent.eliteSpecId) != eliteSpecs.end()) {
                agent.eliteSpec = eliteSpecs[agent.eliteSpecId];
            }
            else {
                agent.eliteSpec = "Core " + agent.profession;
            }
            agentsByAddress[agent.address] = agent;
        }
        offset += 96;  // Size of agent block
    }
}

void parseCombatEvents(const std::vector<char>& bytes, size_t offset, size_t eventCount,
    std::unordered_map<uint64_t, Agent>& agentsByAddress,
    std::unordered_map<uint16_t, Agent*>& playersBySrcInstid,
    ParsedData& result) {
    uint64_t logStartTime = UINT64_MAX;
    uint64_t logEndTime = 0;
    result.combatStartTime = UINT64_MAX;
    result.combatEndTime = 0;
    uint64_t earliestTime = UINT64_MAX;
    uint64_t latestTime = 0;

    const size_t eventSize = sizeof(CombatEvent);

    // First pass: Collect times, map agents to instance IDs, and process team assignments
    for (size_t i = 0; i < eventCount; ++i) {
        size_t eventOffset = offset + (i * eventSize);
        if (eventOffset + eventSize > bytes.size()) {
            break;
        }

        CombatEvent event;
        std::memcpy(&event, bytes.data() + eventOffset, eventSize);

        earliestTime = std::min(earliestTime, event.time);
        latestTime = std::max(latestTime, event.time);

        switch (static_cast<StateChange>(event.isStateChange)) {
        case StateChange::LogStart:
            logStartTime = event.time;
            break;
        case StateChange::LogEnd:
            logEndTime = event.time;
            break;
        case StateChange::EnterCombat:
            result.combatStartTime = std::min(result.combatStartTime, event.time);
            break;
        case StateChange::ExitCombat:
            result.combatEndTime = std::max(result.combatEndTime, event.time);
            break;
        case StateChange::None:
            if (agentsByAddress.find(event.srcAgent) != agentsByAddress.end()) {
                Agent& agent = agentsByAddress[event.srcAgent];
                agent.id = event.srcInstid;
                playersBySrcInstid[event.srcInstid] = &agent;
            }
            break;
        case StateChange::TeamChange: {
            uint32_t teamID = static_cast<uint32_t>(event.value);
            if (teamID != 0 && agentsByAddress.find(event.srcAgent) != agentsByAddress.end()) {
                auto it = teamIDs.find(teamID);
                if (it != teamIDs.end()) {
                    Agent& agent = agentsByAddress[event.srcAgent];
                    agent.team = it->second;
                }
            }
            break;
        }
        default:
            break;
        }
    }

    if (result.combatStartTime == UINT64_MAX) {
        result.combatStartTime = (logStartTime != UINT64_MAX) ? logStartTime : earliestTime;
    }
    if (result.combatEndTime == 0) {
        result.combatEndTime = (logEndTime != 0) ? logEndTime : latestTime;
    }

    // Second pass: Process deaths and downs events
    for (size_t i = 0; i < eventCount; ++i) {
        size_t eventOffset = offset + (i * eventSize);
        if (eventOffset + eventSize > bytes.size()) {
            break;
        }

        CombatEvent event;
        std::memcpy(&event, bytes.data() + eventOffset, eventSize);

        StateChange stateChange = static_cast<StateChange>(event.isStateChange);
        if (stateChange == StateChange::ChangeDead || stateChange == StateChange::ChangeDown) {
            uint16_t srcInstid = event.srcInstid;
            auto playerIt = playersBySrcInstid.find(srcInstid);
            if (playerIt != playersBySrcInstid.end()) {
                Agent* player = playerIt->second;
                const std::string& team = player->team;
                if (team != "Unknown") {
                    if (stateChange == StateChange::ChangeDead) {
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
    for (size_t i = 0; i < eventCount; ++i) {
        size_t eventOffset = offset + (i * eventSize);
        if (eventOffset + eventSize > bytes.size()) {
            break;
        }

        CombatEvent event;
        std::memcpy(&event, bytes.data() + eventOffset, eventSize);

        if (event.isStateChange == static_cast<uint8_t>(StateChange::None) &&
            event.isActivation == static_cast<uint8_t>(Activation::None) &&
            event.isBuffRemove == static_cast<uint8_t>(BuffRemove::None)) {

            ResultCode resultCode = static_cast<ResultCode>(event.result);
            if (resultCode == ResultCode::Normal || resultCode == ResultCode::Critical ||
                resultCode == ResultCode::Glance || resultCode == ResultCode::KillingBlow) {

                int32_t damageValue = 0;
                if (event.buff == 0) {
                    damageValue = event.value;
                }
                else if (event.buff == 1) {
                    damageValue = event.buffDmg;
                }

                if (damageValue > 0) {
                    auto srcIt = playersBySrcInstid.find(event.srcInstid);
                    if (srcIt != playersBySrcInstid.end()) {
                        Agent* agent = srcIt->second;
                        const std::string& team = agent->team;
                        if (team != "Unknown") {
                            // Accumulate total damage for the team
                            result.teamStats[team].totalDamage += damageValue;

                            // Accumulate damage per specialization
                            result.teamStats[team].eliteSpecStats[agent->eliteSpec].totalDamage += damageValue;
                        }
                    }
                }
            }
        }
    }

    // Collect statistics (same as before)
    for (const auto& [srcInstid, player] : playersBySrcInstid) {
        if (player->team != "Unknown") {
            result.teamStats[player->team].totalPlayers++;
            result.teamStats[player->team].eliteSpecStats[player->eliteSpec].count++;
        }
        else {
            APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
                ("Player with unknown team - srcInstid: " + std::to_string(srcInstid) +
                    ", Name: " + player->name).c_str());
        }
    }

    // Log the totals (same as before)
    for (const auto& [teamName, stats] : result.teamStats) {
        std::string message = "Team: " + teamName +
            ", Total Damage: " + std::to_string(stats.totalDamage) +
            ", Total Players: " + std::to_string(stats.totalPlayers) +
            ", Total Downs: " + std::to_string(stats.totalDowned) +
            ", Total Deaths: " + std::to_string(stats.totalDeaths);
        APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, message.c_str());

        // Log per specialization
        for (const auto& [eliteSpec, specStats] : stats.eliteSpecStats) {
            std::string specMessage = "  Elite Spec: " + eliteSpec +
                ", Count: " + std::to_string(specStats.count) +
                ", Total Damage: " + std::to_string(specStats.totalDamage);
            APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, specMessage.c_str());
        }
    }

    APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
        ("Parsed EVTC file. Found " + std::to_string(playersBySrcInstid.size()) + " unique players").c_str());
}


ParsedData parseEVTCFile(const std::string& filePath) {
    ParsedData result;
    std::vector<char> bytes = extractZipFile(filePath);
    if (bytes.empty()) {
        APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, "Failed to extract EVTC file");
        return result;
    }

    size_t offset = 16;  // Skip header (16 bytes)

    // Parse agents
    uint32_t agentCount;
    std::memcpy(&agentCount, bytes.data() + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    std::unordered_map<uint64_t, Agent> agentsByAddress;
    parseAgents(bytes, offset, agentCount, agentsByAddress);

    // Skip skills
    uint32_t skillCount;
    std::memcpy(&skillCount, bytes.data() + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t) + (68 * skillCount);

    // Parse combat events
    const size_t eventSize = sizeof(CombatEvent);
    size_t remainingBytes = bytes.size() - offset;
    size_t eventCount = remainingBytes / eventSize;

    std::unordered_map<uint16_t, Agent*> playersBySrcInstid;
    parseCombatEvents(bytes, offset, eventCount, agentsByAddress, playersBySrcInstid, result);

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