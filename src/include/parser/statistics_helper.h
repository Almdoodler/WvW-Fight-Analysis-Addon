#pragma once

#include "evtc_parser.h"
#include "shared/Shared.h"
#include <algorithm> 
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <string>

/**
 * @brief Check if a boon is tracked for statistics purposes
 * @param skillId The skill ID to check
 * @return True if the boon should be tracked, false otherwise
 */
bool isTrackedBoon(uint32_t skillId);

/**
 * @brief Update team statistics based on combat events
 * @param teamStats The team statistics to update
 * @param agent The agent involved in the event
 * @param value The value to update (damage amount, etc.)
 * @param isDamage Whether this update involves damage
 * @param isKill Whether this update involves a kill
 * @param vsPlayer Whether this update is against a player
 * @param isStrikeDamage Whether this update involves strike damage
 * @param isCondiDamage Whether this update involves condition damage
 * @param isDownedContribution Whether this update contributes to downing
 * @param isKillContribution Whether this update contributes to killing
 * @param isStrip Whether this update involves a boon strip
 */
void updateStats(TeamStats& teamStats, Agent* agent, int32_t value, bool isDamage, bool isKill, bool vsPlayer,
    bool isStrikeDamage, bool isCondiDamage, bool isDownedContribution, bool isKillContribution, bool isStrip);

/**
 * @brief Check if damage occurred as part of a sequence leading to a down
 * @param agent The agent that might be downed
 * @param state The state of the agent
 * @param currentTime The current combat time
 * @return True if the damage contributes to a downing sequence
 */
bool isDamageInDownSequence(const Agent* agent, const AgentState& state, uint64_t currentTime);

/**
 * @brief Check if damage occurred as part of a sequence leading to a kill
 * @param agent The agent that might be killed
 * @param state The state of the agent
 * @param currentTime The current combat time
 * @return True if the damage contributes to a kill sequence
 */
bool isDamageInKillSequence(const Agent* agent, const AgentState& state, uint64_t currentTime);