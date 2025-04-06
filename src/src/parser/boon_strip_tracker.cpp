#include "parser/boon_strip_tracker.h"
#include <algorithm>

void BoonStripTracker::trackSkillActivation(uint16_t srcInstid, uint32_t skillId, uint64_t timestamp) {
    if (knownBoonStripSkills.count(skillId) > 0) {
        playerSkillActivations[srcInstid].push_back(timestamp);
    }
}

bool BoonStripTracker::hasRecentStripSkill(uint16_t instid, uint64_t currentTime, uint64_t windowMs) {
    if (windowMs == 0) windowMs = DEFAULT_WINDOW_MS;

    auto it = playerSkillActivations.find(instid);
    if (it == playerSkillActivations.end()) return false;

    // Check from most recent activations first (optimization)
    for (auto timeIt = it->second.rbegin(); timeIt != it->second.rend(); ++timeIt) {
        uint64_t activationTime = *timeIt;

        // Within the temporal window?
        if (currentTime >= activationTime && currentTime - activationTime <= windowMs) {
            return true;
        }

        // Beyond our window, no need to check older activations
        if (currentTime - activationTime > windowMs) {
            break;
        }
    }
    return false;
}

void BoonStripTracker::cleanupOldActivations(uint64_t currentTime, uint64_t expirationMs) {
    // Only clean up periodically for performance
    if (expirationMs == 0) expirationMs = CLEANUP_INTERVAL_MS;

    if (lastCleanupTime == 0 || currentTime - lastCleanupTime >= CLEANUP_INTERVAL_MS) {
        for (auto& [instid, activations] : playerSkillActivations) {
            activations.erase(
                std::remove_if(activations.begin(), activations.end(),
                    [currentTime, expirationMs](uint64_t activationTime) {
                        return currentTime - activationTime > expirationMs;
                    }),
                activations.end());
        }
        lastCleanupTime = currentTime;
    }
}