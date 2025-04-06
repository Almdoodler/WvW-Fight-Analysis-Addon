// boon_strip_skills.cpp
#include "parser/boon_strip_skills.h"

// Define the set (only once in the program)
const std::unordered_set<uint32_t> knownBoonStripSkills = {
    10602,  // Corrupt Boon (Necro)
    10698,  // Null Field (Mesmer)
    30792,  // Break Enchantments (Warrior)
    30088,  // Winds of Disenchantment (Spellbreaker)
    // Add more known boon strip skills as identified
};