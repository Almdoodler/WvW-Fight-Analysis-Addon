#ifndef SETTINGS_H
#define SETTINGS_H

#include <mutex>

#include "nlohmann/json.hpp"
using json = nlohmann::json;

extern const char* IS_ADDON_WIDGET_VISIBLE;
extern const char* IS_ADDON_WINDOW_VISIBLE;

// Options
extern const char* IS_WINDOW_VISIBLE_IN_COMBAT;
extern const char* CUSTOM_LOG_PATH;
extern const char* TEAM_PLAYER_THRESHOLD;
extern const char* LOG_HISTORY_SIZE;

// Display
extern const char* USE_SHORT_CLASS_NAMES;
extern const char* SHOW_CLASS_NAMES;
extern const char* SHOW_CLASS_ICONS;
extern const char* SHOW_SPEC_BARS;
// Team Stats
extern const char* SHOW_TEAM_TOTAL_PLAYERS;
extern const char* SHOW_TEAM_DEATHS;
extern const char* SHOW_TEAM_DOWNED;
extern const char* SHOW_TEAM_DAMAGE;
extern const char* SHOW_TEAM_STRIKE;
extern const char* SHOW_TEAM_CONDI;
// Spec Stats
extern const char* SHOW_SPEC_DAMAGE;
extern const char* SORT_SPEC_DAMAGE;
// Window Style
extern const char* USE_TABBED_VIEW;
extern const char* SHOW_SCROLL_BAR;
extern const char* SHOW_WINDOW_TITLE;

namespace Settings
{
	extern std::mutex	Mutex;
	extern json			Settings;

	/* Loads the settings. */
	void Load(std::filesystem::path aPath);
	/* Saves the settings. */
	void Save(std::filesystem::path aPath);

	/* Windows */
	extern bool IsAddonWidgetEnabled;
	extern bool IsAddonWindowEnabled;
	
	//Options
	extern bool showWindowInCombat;
	extern int teamPlayerThreshold;
	extern std::string LogDirectoryPath;
	extern char LogDirectoryPathC[256];
	extern size_t logHistorySize;

	//Display
	extern bool useShortClassNames;
	extern bool showClassNames;
	extern bool showClassIcons;
	extern bool showSpecBars;


	// Team Stats
	extern bool showTeamTotalPlayers;
	extern bool showTeamDeaths;
	extern bool showTeamDowned;
	extern bool showTeamDamage;
	extern bool showTeamCondiDamage;
	extern bool showTeamStrikeDamage;
	// Spec Stats
	extern bool showSpecDamage;
	extern bool sortSpecDamage;
	// Window Style
	extern bool showScrollBar;
	extern bool useTabbedView;
	extern bool showWindowTitle;
}

#endif