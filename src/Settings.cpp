#include "Settings.h"

#include "Shared.h"

#include <filesystem>
#include <fstream>

const char* IS_ADDON_WINDOW_VISIBLE = "IsWindowVisible";
const char* IS_WINDOW_VISIBLE_IN_COMBAT = "IsWindowVisibleInCombat";
const char* CUSTOM_LOG_PATH = "CustomLogDirectoryPath";
const char* TEAM_PLAYER_THRESHOLD = "TeamPlayerThreshold";
const char* SHOW_CLASS_NAMES = "ShowClassNames";
const char* USE_SHORT_CLASS_NAMES = "UseShortClassNames";
const char* SHOW_CLASS_ICONS = "ShowClassIcons";

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
		/* Widget */
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

	/* Global */

	/* Banner */
	bool IsAddonWindowEnabled = true;
	bool showWindowInCombat = true;
	int teamPlayerThreshold = 1;
	bool showClassNames = true;
	bool useShortClassNames = false;
	bool showClassIcons = true;
	std::string LogDirectoryPath;
	char LogDirectoryPathC[256] = "";
}