#include "Settings.h"

#include "Shared.h"

#include <filesystem>
#include <fstream>

const char*  IS_ADDON_WIDGET_VISIBLE = "IsWidgetVisible";
const char* IS_ADDON_WINDOW_VISIBLE = "IsWindowVisible";

// Options
const char* IS_WINDOW_VISIBLE_IN_COMBAT = "IsWindowVisibleInCombat";
const char* CUSTOM_LOG_PATH = "CustomLogDirectoryPath";
const char* TEAM_PLAYER_THRESHOLD = "TeamPlayerThreshold";
const char*  LOG_HISTORY_SIZE = "LogHistorySize";

// Display
const char* SHOW_CLASS_NAMES = "ShowClassNames";
const char* USE_SHORT_CLASS_NAMES = "UseShortClassNames";
const char* SHOW_CLASS_ICONS = "ShowClassIcons";

// Team Stats
const char* SHOW_TEAM_TOTAL_PLAYERS = "ShowTeamTotalPlayers";
const char* SHOW_TEAM_DEATHS = "ShowTeamDeaths";
const char* SHOW_TEAM_DOWNED = "ShowTeamDowned";
const char* SHOW_TEAM_DAMAGE = "ShowTeamDamage";
//Specs
const char* SHOW_SPEC_DAMAGE = "ShowSpecDamage";
const char* SORT_SPEC_DAMAGE = "SortSpecDamage";
// Window Style
const char* SHOW_SCROLL_BAR = "ShowScrollBar";


namespace Settings
{
	std::mutex	Mutex;
	json		Settings = json::object();

	void Load(std::filesystem::path aPath)
	{
		if (!std::filesystem::exists(aPath)) { return; }

		Settings::Mutex.lock();
		{
			try
			{
				std::ifstream file(aPath);
				Settings = json::parse(file);
				file.close();
			}
			catch (json::parse_error& ex)
			{

			}
		}
		Settings::Mutex.unlock();
		/* Widget */
		if (!Settings[IS_ADDON_WIDGET_VISIBLE].is_null())
		{
			Settings[IS_ADDON_WIDGET_VISIBLE].get_to<bool>(IsAddonWidgetEnabled);
		}
		/* Window */
		if (!Settings[IS_ADDON_WINDOW_VISIBLE].is_null())
		{
			Settings[IS_ADDON_WINDOW_VISIBLE].get_to<bool>(IsAddonWindowEnabled);
		}
		if (!Settings[IS_WINDOW_VISIBLE_IN_COMBAT].is_null())
		{
			Settings[IS_WINDOW_VISIBLE_IN_COMBAT].get_to<bool>(showWindowInCombat);
		}
		if (!Settings[SHOW_CLASS_NAMES].is_null())
		{
			Settings[SHOW_CLASS_NAMES].get_to<bool>(showClassNames);
		}
		if (!Settings[USE_SHORT_CLASS_NAMES].is_null())
		{
			Settings[USE_SHORT_CLASS_NAMES].get_to<bool>(useShortClassNames);
		}
		if (!Settings[SHOW_CLASS_ICONS].is_null())
		{
			Settings[SHOW_CLASS_ICONS].get_to<bool>(showClassIcons);
		}
		if (!Settings[TEAM_PLAYER_THRESHOLD].is_null())
		{
			Settings[TEAM_PLAYER_THRESHOLD].get_to(teamPlayerThreshold);
		}
		if (!Settings[CUSTOM_LOG_PATH].is_null())
		{
			Settings[CUSTOM_LOG_PATH].get_to<std::string>(LogDirectoryPath);
			strcpy_s(LogDirectoryPathC, sizeof(LogDirectoryPathC), LogDirectoryPath.c_str());
		}
		if (!Settings[LOG_HISTORY_SIZE].is_null())
		{
			Settings[LOG_HISTORY_SIZE].get_to(logHistorySize);
		}
		/* Team Stats */
		if (!Settings[SHOW_TEAM_TOTAL_PLAYERS].is_null())
		{
			Settings[SHOW_TEAM_TOTAL_PLAYERS].get_to<bool>(showTeamTotalPlayers);
		}
		if (!Settings[SHOW_TEAM_DEATHS].is_null())
		{
			Settings[SHOW_TEAM_DEATHS].get_to<bool>(showTeamDeaths);
		}
		if (!Settings[SHOW_TEAM_DOWNED].is_null())
		{
			Settings[SHOW_TEAM_DOWNED].get_to<bool>(showTeamDowned);
		}
		if (!Settings[SHOW_TEAM_DAMAGE].is_null())
		{
			Settings[SHOW_TEAM_DAMAGE].get_to<bool>(showTeamDamage);
		}
		/* Spec Stats */
		if (!Settings[SHOW_SPEC_DAMAGE].is_null())
		{
			Settings[SHOW_SPEC_DAMAGE].get_to<bool>(showSpecDamage);
		}
		if (!Settings[SORT_SPEC_DAMAGE].is_null())
		{
			Settings[SORT_SPEC_DAMAGE].get_to<bool>(sortSpecDamage);
		}
		/* Window Style */
		if (!Settings[SHOW_SCROLL_BAR].is_null())
		{
			Settings[SHOW_SCROLL_BAR].get_to<bool>(showScrollBar);
		}

	}
	void Save(std::filesystem::path aPath)
	{
		Settings::Mutex.lock();
		{
			std::ofstream file(aPath);
			file << Settings.dump(1, '\t') << std::endl;
			file.close();
		}
		Settings::Mutex.unlock();
	}

	/* Windows */

	bool IsAddonWidgetEnabled = true;
	bool IsAddonWindowEnabled = true;
	
	/* Options */

	bool showWindowInCombat = true;
	int teamPlayerThreshold = 1;
;
	std::string LogDirectoryPath;
	char LogDirectoryPathC[256] = "";
	size_t logHistorySize = 10;

	// Display
	bool showClassNames = true;
	bool useShortClassNames = false;
	bool showClassIcons = true;
	// Team Stats
	bool showTeamTotalPlayers = true;
	bool showTeamDeaths = true;
	bool showTeamDowned = true;
	bool showTeamDamage = true;
	// Spec Stats
	bool showSpecDamage = true;
	bool sortSpecDamage = false;
	// Window Style
	bool showScrollBar = true;
}