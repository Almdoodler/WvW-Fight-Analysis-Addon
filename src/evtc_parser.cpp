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
#include <mutex>

std::unordered_set<std::wstring> processedFiles;
std::filesystem::file_time_type maxProcessedTime = std::filesystem::file_time_type::min();


void parseAgents(const std::vector<char>& bytes, size_t& offset, uint32_t agentCount,
	std::unordered_map<uint64_t, Agent>& agentsByAddress) {
	APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, ("Agent count: " + std::to_string(agentCount)).c_str());

	const size_t agentBlockSize = 96; // Each agent block is 96 bytes

	for (uint32_t i = 0; i < agentCount; ++i) {
		if (offset + agentBlockSize > bytes.size()) {
			APIDefs->Log(ELogLevel_WARNING, ADDON_NAME, "Insufficient data for agent block");
			break;
		}

		Agent agent;
		std::memcpy(&agent.address, bytes.data() + offset, sizeof(uint64_t));
		std::memcpy(&agent.professionId, bytes.data() + offset + 8, sizeof(uint32_t));
		std::memcpy(&agent.eliteSpecId, bytes.data() + offset + 12, sizeof(int32_t));

		agent.name = std::string(bytes.data() + offset + 28, 68);
		agent.name.erase(std::find(agent.name.begin(), agent.name.end(), '\0'), agent.name.end());

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
		offset += agentBlockSize;
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

	// Create a mapping from instance IDs to agents
	std::unordered_map<uint16_t, Agent*> agentsByInstid;

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
			// Map srcInstid
			if (agentsByAddress.find(event.srcAgent) != agentsByAddress.end()) {
				Agent& agent = agentsByAddress[event.srcAgent];
				agent.id = event.srcInstid;
				agentsByInstid[event.srcInstid] = &agent;
				playersBySrcInstid[event.srcInstid] = &agent; // Keep this mapping
			}
			// Map dstInstid
			if (agentsByAddress.find(event.dstAgent) != agentsByAddress.end()) {
				Agent& agent = agentsByAddress[event.dstAgent];
				agent.id = event.dstInstid;
				agentsByInstid[event.dstInstid] = &agent;
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
			auto agentIt = agentsByInstid.find(srcInstid);
			if (agentIt != agentsByInstid.end()) {
				Agent* agent = agentIt->second;
				const std::string& team = agent->team;
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

			// Check for damage events
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
						Agent* attacker = srcIt->second;
						const std::string& team = attacker->team;
						if (team != "Unknown") {
							// Accumulate total damage
							if (event.buff == 0) {
								// Accumulate strike damage
								result.teamStats[team].totalStrikeDamage += damageValue;
								result.teamStats[team].eliteSpecStats[attacker->eliteSpec].totalStrikeDamage += damageValue;
							}
							else {
								// Accumulate condition damage
								result.teamStats[team].totalCondiDamage += damageValue;
								result.teamStats[team].eliteSpecStats[attacker->eliteSpec].totalCondiDamage += damageValue;
							}
							result.teamStats[team].totalDamage += damageValue;
							result.teamStats[team].eliteSpecStats[attacker->eliteSpec].totalDamage += damageValue;

							// Check if target is a logged player
							auto dstIt = agentsByInstid.find(event.dstInstid);
							if (dstIt != agentsByInstid.end()) {
								Agent* target = dstIt->second;
								if (target->team != "Unknown") {
									// Accumulate damage vs. logged players
									if (event.buff == 0) {
										result.teamStats[team].totalStrikeDamageVsPlayers += damageValue;
										result.teamStats[team].eliteSpecStats[attacker->eliteSpec].totalStrikeDamageVsPlayers += damageValue;
									}
									else {
										result.teamStats[team].totalCondiDamageVsPlayers += damageValue;
										result.teamStats[team].eliteSpecStats[attacker->eliteSpec].totalCondiDamageVsPlayers += damageValue;
									}
									result.teamStats[team].totalDamageVsPlayers += damageValue;
									result.teamStats[team].eliteSpecStats[attacker->eliteSpec].totalDamageVsPlayers += damageValue;
								}
							}
						}
					}
				}
			}

			// Process KillingBlow events to collect kills per team
			if (resultCode == ResultCode::KillingBlow) {
				auto srcIt = playersBySrcInstid.find(event.srcInstid);
				if (srcIt != playersBySrcInstid.end()) {
					Agent* attacker = srcIt->second;
					const std::string& team = attacker->team;
					if (team != "Unknown") {
						// Increment total kills
						result.teamStats[team].totalKills++;
						result.teamStats[team].eliteSpecStats[attacker->eliteSpec].totalKills++;

						// Check if target is a logged player
						auto dstIt = agentsByInstid.find(event.dstInstid);
						if (dstIt != agentsByInstid.end()) {
							Agent* target = dstIt->second;
							if (target->team != "Unknown") {
								// Increment kills vs. logged players
								result.teamStats[team].totalKillsVsPlayers++;
								result.teamStats[team].eliteSpecStats[attacker->eliteSpec].totalKillsVsPlayers++;
							}
						}
					}
				}
			}
		}
	}

	// Collect statistics
	for (const auto& [srcInstid, agent] : playersBySrcInstid) {
		if (agent->team != "Unknown") {
			result.teamStats[agent->team].totalPlayers++;
			result.teamStats[agent->team].eliteSpecStats[agent->eliteSpec].count++;
			result.totalIdentifiedPlayers++;
		}
		else {
			APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
				("Agent with unknown team - InstID: " + std::to_string(srcInstid) +
					", Name: " + agent->name).c_str());
		}
	}

	// Log the totals
	for (const auto& [teamName, stats] : result.teamStats) {
		std::string message = "Team: " + teamName +
			", Total Damage: " + std::to_string(stats.totalDamage) +
			", Damage vs Players: " + std::to_string(stats.totalDamageVsPlayers) +
			", Total Players: " + std::to_string(stats.totalPlayers) +
			", Total Downs: " + std::to_string(stats.totalDowned) +
			", Total Deaths: " + std::to_string(stats.totalDeaths) +
			", Total Kills: " + std::to_string(stats.totalKills) +
			", Kills vs Players: " + std::to_string(stats.totalKillsVsPlayers);
		APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, message.c_str());

		// Log per specialization
		for (const auto& [eliteSpec, specStats] : stats.eliteSpecStats) {
			std::string specMessage = "  Elite Spec: " + eliteSpec +
				", Count: " + std::to_string(specStats.count) +
				", Total Damage: " + std::to_string(specStats.totalDamage) +
				", Damage vs Players: " + std::to_string(specStats.totalDamageVsPlayers) +
				", Total Kills: " + std::to_string(specStats.totalKills) +
				", Kills vs Players: " + std::to_string(specStats.totalKillsVsPlayers);
			APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, specMessage.c_str());
		}
	}

	APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
		("Parsed EVTC file. Found " + std::to_string(result.totalIdentifiedPlayers) + " identified players").c_str());
}






ParsedData parseEVTCFile(const std::string& filePath) {
	ParsedData result;
	std::vector<char> bytes = extractZipFile(filePath);
	if (bytes.size() < 16) {
		APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, "EVTC file is too small");
		return result;
	}

	size_t offset = 0;

	// Read header (12 bytes)
	char header[13] = { 0 };
	std::memcpy(header, bytes.data() + offset, 12);
	offset += 12;

	// Read revision (1 byte)
	uint8_t revision;
	std::memcpy(&revision, bytes.data() + offset, sizeof(uint8_t));
	offset += sizeof(uint8_t);

	// Read fight instance ID (uint16_t)
	uint16_t fightId;
	std::memcpy(&fightId, bytes.data() + offset, sizeof(uint16_t));
	offset += sizeof(uint16_t);

	// skip (1 byte)
	uint8_t skipByte;
	std::memcpy(&skipByte, bytes.data() + offset, sizeof(uint8_t));
	offset += sizeof(uint8_t);

	std::string headerStr(header);

	if (headerStr.rfind("EVTC", 0) != 0 ||
		!std::all_of(headerStr.begin() + 4, headerStr.end(), ::isdigit)) {
		APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, ("Not EVTC " + headerStr).c_str());
		return ParsedData(); // Return an empty result
	}

	APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, ("Header: " + headerStr + ", Revision: " + std::to_string(revision) + ", Fight Instance ID: " + std::to_string(fightId)).c_str());
	int evtcVersion = std::stoi(headerStr.substr(4));

	if (evtcVersion < 20240612) {
		APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, ("Cannot parse EVTC Version is pre TeamChangeOnDespawn / 20240612"));
		return ParsedData(); // Return an empty result
	}

	// Check if fightId is 1 (WvW)
	if (fightId != 1) {
		APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, ("Skipping non-WvW log. FightInstanceID: " + std::to_string(fightId)).c_str());
		return ParsedData(); // Return an empty result
	}
	

	result.fightId = fightId;

	// Read agent count (uint32_t)
	if (offset + sizeof(uint32_t) > bytes.size()) {
		APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, "Incomplete EVTC file: Missing agent count");
		return result;
	}
	uint32_t agentCount;
	std::memcpy(&agentCount, bytes.data() + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	std::unordered_map<uint64_t, Agent> agentsByAddress;
	parseAgents(bytes, offset, agentCount, agentsByAddress);

	// Read skill count (uint32_t)
	if (offset + sizeof(uint32_t) > bytes.size()) {
		APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, "Incomplete EVTC file: Missing skill count");
		return result;
	}
	uint32_t skillCount;
	std::memcpy(&skillCount, bytes.data() + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	// Skip skills (68 bytes per skill)
	size_t skillsSize = 68 * skillCount;
	if (offset + skillsSize > bytes.size()) {
		APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, "Incomplete EVTC file: Skills data missing");
		return result;
	}
	offset += skillsSize;

	// Parse combat events
	const size_t eventSize = sizeof(CombatEvent);
	size_t remainingBytes = bytes.size() - offset;
	size_t eventCount = remainingBytes / eventSize;

	std::unordered_map<uint16_t, Agent*> playersBySrcInstid;
	parseCombatEvents(bytes, offset, eventCount, agentsByAddress, playersBySrcInstid, result);


	return result;
}


std::wstring getCanonicalPath(const std::filesystem::path& path)
{
	return std::filesystem::weakly_canonical(path).wstring();
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


void processEVTCFile(const std::filesystem::path& filePath)
{
	waitForFile(filePath.string());
	processNewEVTCFile(filePath.string());
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

		// Recursively search for .zevtc files in the directory
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

		// Sort escending order (newest first)
		std::sort(zevtcFiles.begin(), zevtcFiles.end(),
			[](const std::filesystem::path& a, const std::filesystem::path& b)
			{
				return std::filesystem::last_write_time(a) > std::filesystem::last_write_time(b);
			});

		size_t numFilesToParse = std::min<size_t>(zevtcFiles.size(), numLogsToParse);

		for (size_t i = 0; i < numFilesToParse; ++i)
		{
			const std::filesystem::path& filePath = zevtcFiles[i];
			waitForFile(filePath.string());

			std::wstring absolutePath = std::filesystem::absolute(filePath).wstring();

			if (processedFiles.find(absolutePath) == processedFiles.end())
			{
				// Process the file
				ParsedLog log;
				log.filename = filePath.filename().string();
				log.data = parseEVTCFile(filePath.string());

				// fightId 1 = WvW
				if (log.data.fightId != 1)
				{
					APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
						("Skipping non-WvW log during initial parsing: " + log.filename).c_str());
					continue;
				}

				if (log.data.totalIdentifiedPlayers == 0)
				{
					APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
						("Skipping log with no identified players during initial parsing: " + log.filename).c_str());
					continue;
				}

				// Add to parsed logs
				parsedLogs.push_back(log);

				while (parsedLogs.size() > Settings::logHistorySize)
				{
					parsedLogs.pop_front();
				}
				currentLogIndex = 0;

				processedFiles.insert(absolutePath);

				// In the Wine implementation we will only parse logs with newer time stamps.
				// Not quite the same behaviour but good enough
				auto fileTime = std::filesystem::last_write_time(filePath);
				if (fileTime > maxProcessedTime)
				{
					maxProcessedTime = fileTime;
				}
			}
			else
			{
				APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
					("File already processed during initial parsing: " + filePath.string()).c_str());
			}
		}

		initialParsingComplete = true;
	}
	catch (const std::exception& ex)
	{
		APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
			("Exception in initial parsing: " + std::string(ex.what())).c_str());
	}
}

void scanForNewFiles(const std::filesystem::path& dirPath, std::unordered_set<std::wstring>& processedFiles)
{
	try
	{
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

		// Sort the files by last write time in descending order
		std::sort(zevtcFiles.begin(), zevtcFiles.end(),
			[](const std::filesystem::path& a, const std::filesystem::path& b)
			{
				return std::filesystem::last_write_time(a) > std::filesystem::last_write_time(b);
			});

		for (const auto& filePath : zevtcFiles)
		{
			// Get the last write time of the file
			auto fileTime = std::filesystem::last_write_time(filePath);

			// Only process files newer than maxProcessedTime
			if (fileTime <= maxProcessedTime)
			{
				break; // Since files are sorted descending, no need to check older files
			}

			std::wstring absolutePath = std::filesystem::absolute(filePath).wstring();

			if (processedFiles.find(absolutePath) != processedFiles.end())
			{
				continue; // Already processed
			}


			processEVTCFile(filePath.string());

			// Add the absolute path to the set of processed files
			processedFiles.insert(absolutePath);

			// Update maxProcessedTime
			if (fileTime > maxProcessedTime)
			{
				maxProcessedTime = fileTime;
			}
		}
	}
	catch (const std::exception& ex)
	{
		APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
			("Exception in scanning directory: " + std::string(ex.what())).c_str());
	}
}

// ReadDirectoryChangesW doesn't seem to work under Wine, we'll keep using this version on Windows though
void directoryMonitor(const std::filesystem::path& dirPath, size_t numLogsToParse)
{
	try
	{
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

		parseInitialLogs(processedFiles, numLogsToParse);

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
								std::wstring fullPathStr = std::filesystem::absolute(fullPath).wstring();

								{
									std::lock_guard<std::mutex> lock(processedFilesMutex);

									if (processedFiles.find(fullPathStr) == processedFiles.end())
									{
										processEVTCFile(fullPath);

										processedFiles.insert(fullPathStr);
									}
								}
							}
						}

						if (fni->NextEntryOffset == 0)
							break;

						fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
							reinterpret_cast<BYTE*>(fni) + fni->NextEntryOffset);
					} while (true);

					ResetEvent(overlapped.hEvent);

					// Reissue the ReadDirectoryChangesW call
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
							"Failed to reissue directory monitoring.");
						break;
					}
				}
				else
				{
					DWORD errorCode = GetLastError();
					APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
						("GetOverlappedResult failed with error code: " + std::to_string(errorCode)).c_str());
					break;
				}
			}
			else if (waitStatus == WAIT_TIMEOUT)
			{
				// Continue waiting
				continue;
			}
			else
			{
				DWORD errorCode = GetLastError();
				APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
					("WaitForSingleObject failed with error code: " + std::to_string(errorCode)).c_str());
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


void monitorDirectory(size_t numLogsToParse, size_t pollIntervalMilliseconds)
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

		// Determine if we're running under Wine
		bool runningUnderWine = isRunningUnderWine();
		if (runningUnderWine)
		{
			APIDefs->Log(ELogLevel_INFO, ADDON_NAME, "Running under Wine/Proton. Using polling method.");

			parseInitialLogs(processedFiles, numLogsToParse);

			while (!stopMonitoring)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(pollIntervalMilliseconds));

				if (stopMonitoring)
				{
					break;
				}

				// Poll
				scanForNewFiles(dirPath, processedFiles);
			}
		}
		else
		{
			APIDefs->Log(ELogLevel_INFO, ADDON_NAME, "Running under Windows. Using ReadDirectoryChangesW method.");

			// Use the ReadDirectoryChangesW method
			directoryMonitor(dirPath, numLogsToParse);
		}
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

	if (log.data.fightId != 1)
	{
		APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, ("Skipping non-WvW log: " + filename).c_str());
		return;
	}

	if (log.data.totalIdentifiedPlayers == 0)
	{
		APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, ("Skipping log with no identified players: " + filename).c_str());
		return;
	}

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



