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
extern Texture* Condi;
extern Texture* Strike;


// New declarations
extern std::atomic<bool> initialParsingComplete;
extern std::atomic<bool> stopMonitoring;
extern std::mutex parsedLogsMutex;
extern std::mutex processedFilesMutex;
extern std::thread initialParsingThread;
extern std::thread directoryMonitorThread;
extern int currentLogIndex;


struct TextureInfo {
    int resourceId;
    Texture** texture;
};

// Structures
struct Agent {
    uint64_t address;
    uint32_t professionId;
    int32_t eliteSpecId;
    uint16_t id = 0;
    std::string name;
    std::string profession;
    std::string eliteSpec;
    std::string team = "Unknown";
};

struct SpecStats {
    int count = 0;
    uint64_t totalDamage = 0;
    uint64_t totalStrikeDamage = 0;
    uint64_t totalCondiDamage = 0;
};

struct TeamStats {
    uint32_t totalPlayers = 0;
    uint32_t totalDeaths = 0;
    uint32_t totalDowned = 0;
    uint64_t totalDamage = 0;
    uint64_t totalStrikeDamage = 0;
    uint64_t totalCondiDamage = 0;
    std::unordered_map<std::string, SpecStats> eliteSpecStats;
};

struct ParsedData {
    std::unordered_map<std::string, TeamStats> teamStats;
    uint64_t combatStartTime = 0;
    uint64_t combatEndTime = 0;
    uint16_t fightId = 0;
    size_t totalIdentifiedPlayers = 0;

    double getCombatDurationSeconds() const {
        if (combatEndTime > combatStartTime) {
            return (combatEndTime - combatStartTime) / 1000.0;
        }
        return 0.0; 
    }
};

struct ParsedLog {
    std::string filename;
    ParsedData data;
};

extern std::deque<ParsedLog> parsedLogs;

// Combat Event Structure
#pragma pack(push, 1)  // Disable padding
struct CombatEvent {
    uint64_t time;
    uint64_t srcAgent;
    uint64_t dstAgent;
    int32_t value;
    int32_t buffDmg;
    uint32_t overstackValue;
    uint32_t skillId;
    uint16_t srcInstid;
    uint16_t dstInstid;
    uint16_t srcMasterInstid;
    uint16_t dstMasterInstid;
    uint8_t iff;
    uint8_t buff;
    uint8_t result;
    uint8_t isActivation;
    uint8_t isBuffRemove;
    uint8_t isNinety;
    uint8_t isFifty;
    uint8_t isMoving;
    uint8_t isStateChange;
    uint8_t isFlanking;
    uint8_t isShields;
    uint8_t isOffCycle;
    uint32_t pad;
};
#pragma pack(pop)


// enum

enum class StateChange : uint8_t {
    None = 0,
    EnterCombat = 1,
    ExitCombat = 2,
    ChangeUp = 3,
    ChangeDead = 4,
    ChangeDown = 5,
    Spawn = 6,
    Despawn = 7,
    HealthUpdate = 8,
    LogStart = 9,
    LogEnd = 10,
    WeaponSwap = 11,
    MaxHealthUpdate = 12,
    PointOfView = 13,
    Language = 14,
    GWBuild = 15,
    ShardId = 16,
    Reward = 17,
    BuffInitial = 18,
    Position = 19,
    Velocity = 20,
    Facing = 21,
    TeamChange = 22,
    AttackTarget = 23,
    Targetable = 24,
    MapID = 29,
    ReplInfo = 25,
    StackActive = 26,
    StackReset = 27,
    Guild = 28,
    Error = 0xFF
};

enum class ResultCode : uint8_t {
    Normal = 0,
    Critical = 1,
    Glance = 2,
    Block = 3,
    Evade = 4,
    Interrupt = 5,
    Absorb = 6,
    Blind = 7,
    KillingBlow = 8,
    Downed = 9
};

enum class Activation : uint8_t {
    None = 0,
    Normal = 1,
    Quickness = 2,
    CancelFire = 3,
    CancelCancel = 4,
    Reset = 5
};

enum class BuffRemove : uint8_t {
    None = 0,
    All = 1,
    Single = 2,
    Manual = 3
};

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