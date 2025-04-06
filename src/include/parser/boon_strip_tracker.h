// boon_strip_tracker.h
#ifndef BOON_STRIP_TRACKER_H
#define BOON_STRIP_TRACKER_H

#include "shared/Shared.h" // Include this first
#include "boon_strip_skills.h"
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <algorithm> // For std::remove_if

class BoonStripTracker {
private:
    // Maps player instid to timestamps of strip skill activations
    std::unordered_map<uint16_t, std::vector<uint64_t>> playerSkillActivations;
    const uint64_t DEFAULT_WINDOW_MS = 500; // Default temporal window
    const uint64_t CLEANUP_INTERVAL_MS = 5000; // Cleanup interval
    uint64_t lastCleanupTime = 0;

public:
    // Track a skill activation if it's a known strip skill
    void trackSkillActivation(uint16_t srcInstid, uint32_t skillId, uint64_t timestamp);

    // Check if a player has recently used a strip skill
    bool hasRecentStripSkill(uint16_t instid, uint64_t currentTime, uint64_t windowMs = 0);

    // Periodically clean up old activations
    void cleanupOldActivations(uint64_t currentTime, uint64_t expirationMs = 0);
};

#endif // BOON_STRIP_TRACKER_H