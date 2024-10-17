#ifndef SETTINGS_H
#define SETTINGS_H

#include <mutex>

#include "nlohmann/json.hpp"
using json = nlohmann::json;

extern const char* IS_ADDON_WIDGET_VISIBLE;
extern const char* IS_ADDON_WINDOW_VISIBLE;
extern const char* IS_WINDOW_VISIBLE_IN_COMBAT;
extern const char* CUSTOM_LOG_PATH;
extern const char* TEAM_PLAYER_THRESHOLD;
extern const char* USE_SHORT_CLASS_NAMES;
extern const char* SHOW_CLASS_NAMES;
extern const char* SHOW_CLASS_ICONS;

// Team Stats
extern const char* SHOW_TEAM_TOTAL_PLAYERS;
extern const char* SHOW_TEAM_DEATHS;
extern const char* SHOW_TEAM_DOWNED;
extern const char* SHOW_TEAM_DAMAGE;
// Spec Stats
extern const char* SHOW_SPEC_DAMAGE;
extern const char* SORT_SPEC_DAMAGE;
// Window Style
extern const char* SHOW_SCROLL_BAR;

namespace Settings
{
	extern std::mutex	Mutex;
	extern json			Settings;

	/* Loads the settings. */
	void Load(std::filesystem::path aPath);
	/* Saves the settings. */
	void Save(std::filesystem::path aPath);

	/* Global */
	extern bool IsAddonWidgetEnabled;
	extern bool IsAddonWindowEnabled;
	extern bool showWindowInCombat;
	extern int teamPlayerThreshold;
	extern bool useShortClassNames;
	extern bool showClassNames;
	extern bool showClassIcons;
	extern std::string LogDirectoryPath;
	extern char LogDirectoryPathC[256];

	// Team Stats
	extern bool showTeamTotalPlayers;
	extern bool showTeamDeaths;
	extern bool showTeamDowned;
	extern bool showTeamDamage;
	// Spec Stats
	extern bool showSpecDamage;
	extern bool sortSpecDamage;
	// Window Style
	extern bool showScrollBar;
}

#endif