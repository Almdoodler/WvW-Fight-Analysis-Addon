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

							if (event.buff == 0) 
							{
								result.teamStats[team].totalStrikeDamage += damageValue;
								result.teamStats[team].eliteSpecStats[agent->eliteSpec].totalStrikeDamage += damageValue;
							}
							else 
							{
								result.teamStats[team].totalCondiDamage += damageValue;
								result.teamStats[team].eliteSpecStats[agent->eliteSpec].totalCondiDamage += damageValue;
							}
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

void processEVTCFile(const std::filesystem::path& filePath)
{
	waitForFile(filePath.string());
	processNewEVTCFile(filePath.string());
}


bool isValidEVTCFile(const std::filesystem::path& dirPath, const std::filesystem::path& filePath)
{
	std::filesystem::path relativePath = std::filesystem::relative(filePath.parent_path(), dirPath);

	bool dirPathIsWvW = dirPath.filename().string().find("WvW") != std::string::npos;

	std::vector<std::filesystem::path> components;
	if (!dirPath.has_root_path() && !dirPathIsWvW)
	{
		components.push_back(dirPath.filename());
	}
	for (const auto& part : relativePath)
	{
		components.push_back(part);
	}
	int wvwIndex = -1;
	for (size_t i = 0; i < components.size(); ++i)
	{
		if (components[i].string().find("WvW") != std::string::npos)
		{
			wvwIndex = static_cast<int>(i);
			break;
		}
	}

	if (dirPathIsWvW && wvwIndex == -1)
	{
		wvwIndex = -1;
	}
	else if (wvwIndex == -1)
	{
		return false;
	}

	size_t depthFromWvW = components.size() - (wvwIndex + 1);

	return depthFromWvW <= 2;
}

void parseInitialLogs(std::unordered_set<std::wstring>& processedFiles, size_t numLogsToParse)
{
	try
	{
		std::filesystem::path dirPath;

		if (!Settings::LogDirectoryPath.empty())
		{
			dirPath = std::filesystem::path(Settings::LogDirectoryPath);

			if (!std::filesystem::exists(dirPath) || !std::filesystem::is_directory(dirPath))
			{
				APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
					("Specified log directory does not exist: " + dirPath.string()).c_str());
				return;
			}
		}
		else
		{
			char documentsPath[MAX_PATH];
			if (FAILED(SHGetFolderPath(nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, documentsPath)))
			{
				APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
					"Failed to get path to user's Documents folder.");
				return;
			}

			dirPath = std::filesystem::path(documentsPath) / "Guild Wars 2" / "addons" /
				"arcdps" / "arcdps.cbtlogs";
		}

		std::vector<std::filesystem::path> zevtcFiles;

		for (const auto& entry : std::filesystem::recursive_directory_iterator(dirPath))
		{
			if (entry.is_regular_file() && entry.path().extension() == L".zevtc")
			{
				if (isValidEVTCFile(dirPath, entry.path()))
				{
					zevtcFiles.push_back(entry.path());
				}
			}
		}

		if (zevtcFiles.empty())
		{
			APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
				("No valid .zevtc files found in directory: " + dirPath.string()).c_str());
			return;
		}

		std::sort(zevtcFiles.begin(), zevtcFiles.end(),
			[](const std::filesystem::path& a, const std::filesystem::path& b)
			{
				return std::filesystem::last_write_time(a) > std::filesystem::last_write_time(b);
			});

		size_t numFilesToParse = std::min<size_t>(zevtcFiles.size(), numLogsToParse);

		for (size_t i = 0; i < numFilesToParse; ++i)
		{
			
			const std::filesystem::path& filePath = zevtcFiles[i];
			// Wait for the file to be fully written
			waitForFile(filePath.string());

			// Parse the file
			ParsedLog log;
			log.filename = filePath.filename().string();;
			log.data = parseEVTCFile(filePath.string());
			{
				std::lock_guard<std::mutex> lock(parsedLogsMutex);
				parsedLogs.push_back(log);

				while (parsedLogs.size() > Settings::logHistorySize) {
					parsedLogs.pop_back();
				}
				currentLogIndex = 0;
			}
			processedFiles.insert(filePath.wstring());
		}

		initialParsingComplete = true;
	}
	catch (const std::exception& ex)
	{
		APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
			("Exception in initial parsing: " + std::string(ex.what())).c_str());
	}
}

void monitorDirectory(size_t numLogsToParse)
{
	try
	{
		std::filesystem::path dirPath;

		if (!Settings::LogDirectoryPath.empty())
		{
			dirPath = std::filesystem::path(Settings::LogDirectoryPath);

			if (!std::filesystem::exists(dirPath) || !std::filesystem::is_directory(dirPath))
			{
				APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
					("Specified log directory does not exist: " + dirPath.string()).c_str());
				return;
			}
		}
		else
		{
			char documentsPath[MAX_PATH];
			if (FAILED(SHGetFolderPath(nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, documentsPath)))
			{
				APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
					"Failed to get path to user's Documents folder.");
				return;
			}

			dirPath = std::filesystem::path(documentsPath) / "Guild Wars 2" / "addons" /
				"arcdps" / "arcdps.cbtlogs";
		}

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
			APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
				("Failed to open directory for monitoring: " + dirPath.string()).c_str());
			return;
		}

		char buffer[4096];
		OVERLAPPED overlapped = { 0 };
		overlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

		BOOL success = ReadDirectoryChangesW(
			hDir,
			buffer,
			sizeof(buffer),
			TRUE,
			FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE |
			FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION,
			nullptr,
			&overlapped,
			nullptr);

		if (!success)
		{
			APIDefs->Log(ELogLevel_WARNING, ADDON_NAME, "Failed to start directory monitoring.");
			CloseHandle(overlapped.hEvent);
			CloseHandle(hDir);
			return;
		}

		std::unordered_set<std::wstring> processedFiles;
		parseInitialLogs(processedFiles, numLogsToParse);

		while (!stopMonitoring)
		{
			DWORD waitStatus = WaitForSingleObject(overlapped.hEvent, 500); // Wait with timeout

			if (stopMonitoring)
			{
				CancelIoEx(hDir, &overlapped);
				break;
			}

			if (waitStatus == WAIT_OBJECT_0)
			{
				DWORD bytesTransferred = 0;
				if (GetOverlappedResult(hDir, &overlapped, &bytesTransferred, FALSE))
				{
					BYTE* pBuffer = reinterpret_cast<BYTE*>(buffer);
					FILE_NOTIFY_INFORMATION* fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(pBuffer);

					do
					{
						std::wstring filenameW(fni->FileName, fni->FileNameLength / sizeof(WCHAR));
						std::filesystem::path fullPath = dirPath / filenameW;

						// Filter for .zevtc files
						if ((fni->Action == FILE_ACTION_ADDED ||
							fni->Action == FILE_ACTION_MODIFIED ||
							fni->Action == FILE_ACTION_RENAMED_NEW_NAME) &&
							fullPath.extension() == L".zevtc")
						{
							if (isValidEVTCFile(dirPath, fullPath))
							{
								std::wstring fullPathStr = fullPath.wstring();
								if (processedFiles.find(fullPathStr) == processedFiles.end())
								{
									processEVTCFile(fullPath);
									processedFiles.insert(fullPathStr);
								}
							}
						}

						if (fni->NextEntryOffset == 0)
							break;
						fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
							reinterpret_cast<BYTE*>(fni) + fni->NextEntryOffset);
					} while (true);

					ResetEvent(overlapped.hEvent);
					success = ReadDirectoryChangesW(
						hDir,
						buffer,
						sizeof(buffer),
						TRUE,
						FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE |
						FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION,
						nullptr,
						&overlapped,
						nullptr);

					if (!success)
					{
						APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
							"Failed to read directory changes.");
						break;
					}
				}
				else
				{
					APIDefs->Log(ELogLevel_WARNING, ADDON_NAME, "Failed to get overlapped result.");
					break;
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

		CloseHandle(overlapped.hEvent);
		CloseHandle(hDir);
	}
	catch (const std::exception& ex)
	{
		APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
			("Exception in directory monitoring thread: " + std::string(ex.what())).c_str());
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

		while (parsedLogs.size() > Settings::logHistorySize)
		{
			parsedLogs.pop_back();
		}

		currentLogIndex = 0;
	}
}