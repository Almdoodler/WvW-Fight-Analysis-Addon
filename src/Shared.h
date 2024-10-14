#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <thread>
#include "nexus/Nexus.h"
#include "mumble/Mumble.h"
#include "imgui/imgui.h"
#include <deque>

// Existing declarations
extern HMODULE SelfModule;
extern AddonAPI* APIDefs;
extern HWND GameHandle;
extern Mumble::Data* MumbleLink;
extern NexusLinkData* NexusLink;

extern Texture* Berserker;
extern Texture* Bladesworn;
extern Texture* Catalyst;
extern Texture* Chronomancer;
extern Texture* Daredevil;
extern Texture* Deadeye;
extern Texture* Dragonhunter;
extern Texture* Druid;
extern Texture* Elementalist;
extern Texture* Engineer;
extern Texture* Firebrand;
extern Texture* Guardian;
extern Texture* Harbinger;
extern Texture* Herald;
extern Texture* Holosmith;
extern Texture* Mechanist;
extern Texture* Mesmer;
extern Texture* Mirage;
extern Texture* Necromancer;
extern Texture* Ranger;
extern Texture* Revenant;
extern Texture* Reaper;
extern Texture* Renegade;
extern Texture* Scrapper;
extern Texture* Scourge;
extern Texture* Soulbeast;
extern Texture* Specter;
extern Texture* Spellbreaker;
extern Texture* Tempest;
extern Texture* Thief;
extern Texture* Untamed;
extern Texture* Vindicator;
extern Texture* Virtuoso;
extern Texture* Warrior;
extern Texture* Weaver;
extern Texture* Willbender;
extern Texture* Death;
extern Texture* Downed;
extern Texture* Squad;
extern Texture* Damage;


// New declarations
extern std::atomic<bool> initialParsingComplete;
extern std::atomic<bool> stopMonitoring;
extern std::mutex parsedLogsMutex;
extern std::thread initialParsingThread;
extern std::thread directoryMonitorThread;
extern int currentLogIndex;

// Structures
struct Agent {
    uint64_t address;
    uint16_t id = 0;
    std::string name;
    std::string profession;
    std::string eliteSpec;
    std::string team = "Unknown";
};

struct TeamStats {
    int totalPlayers = 0;
    int totalDeaths = 0;
    int totalDowned = 0;
    uint64_t totalDamage = 0;
    std::unordered_map<std::string, int> eliteSpecCounts;

};

struct ParsedData {
    std::unordered_map<std::string, TeamStats> teamStats;
    uint64_t combatStartTime = 0;
    uint64_t combatEndTime = 0;

    // Method to calculate combat duration in seconds
    double getCombatDurationSeconds() const {
        if (combatEndTime > combatStartTime) {
            return (combatEndTime - combatStartTime) / 1000.0;
        }
        return 0.0;  // Return 0 if end time is not after start time
    }
};

struct ParsedLog {
    std::string filename;
    ParsedData data;
};

extern std::deque<ParsedLog> parsedLogs;

// Maps
extern std::unordered_map<int, std::string> professions;
extern std::unordered_map<int, std::string> eliteSpecs;
extern std::unordered_map<int, std::string> teamIDs;
extern std::unordered_map<std::string, std::string> eliteSpecToProfession;
extern std::unordered_map<std::string, std::string> eliteSpecShortNames;
extern std::unordered_map<std::string, ImVec4> professionColors;
// Constants
extern const char* const ADDON_NAME;
extern const char* KB_TOGGLE_SHOW_WINDOW_LOG_PROOFS;