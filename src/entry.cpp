#include "nexus/Nexus.h"
#include "mumble/Mumble.h"
#include "imgui/imgui.h"
#include "gui/WindowRenderer.h"
#include "shared/Shared.h"
#include "settings/Settings.h"
#include "utils/Utils.h"
#include "evtc_parser.h"


// Function prototypes
void AddonLoad(AddonAPI* aApi);
void AddonUnload();
void AddonRender();
void AddonOptions();
void ProcessKeybinds(const char* aIdentifier, bool aIsRelease);

AddonDefinition AddonDef = {};
std::unique_ptr<wvwfightanalysis::gui::WindowRenderer> g_windowRenderer;
<<<<<<< Updated upstream
<<<<<<< Updated upstream
<<<<<<< Updated upstream
<<<<<<< Updated upstream

=======
>>>>>>> Stashed changes
=======
>>>>>>> Stashed changes
=======
>>>>>>> Stashed changes
=======
>>>>>>> Stashed changes

void AddonLoad(AddonAPI* aApi)
{
    APIDefs = aApi;
    ImGui::SetCurrentContext((ImGuiContext*)APIDefs->ImguiContext);
    ImGui::SetAllocatorFunctions((void* (*)(size_t, void*))APIDefs->ImguiMalloc, (void(*)(void*, void*))APIDefs->ImguiFree);
    MumbleLink = (Mumble::Data*)APIDefs->DataLink.Get("DL_MUMBLE_LINK");
    NexusLink = (NexusLinkData*)APIDefs->DataLink.Get("DL_NEXUS_LINK");
<<<<<<< Updated upstream
<<<<<<< Updated upstream
<<<<<<< Updated upstream
<<<<<<< Updated upstream
<<<<<<< Updated upstream


=======
>>>>>>> Stashed changes
=======
>>>>>>> Stashed changes
=======
>>>>>>> Stashed changes
=======
>>>>>>> Stashed changes
=======
>>>>>>> Stashed changes
    APIDefs->Renderer.Register(ERenderType_OptionsRender, AddonOptions);
    APIDefs->Renderer.Register(ERenderType_Render, AddonRender);
    GW2Root = APIDefs->Paths.GetGameDirectory();
    AddonPath = APIDefs->Paths.GetAddonDirectory("WvWFightAnalysis");
    SettingsPath = APIDefs->Paths.GetAddonDirectory("WvWFightAnalysis/settings.json");
<<<<<<< Updated upstream
<<<<<<< Updated upstream
<<<<<<< Updated upstream
<<<<<<< Updated upstream
<<<<<<< Updated upstream
   
    if (!std::filesystem::exists(AddonPath))
    {
=======
=======
>>>>>>> Stashed changes
=======
>>>>>>> Stashed changes
=======
>>>>>>> Stashed changes
=======
>>>>>>> Stashed changes
    if (!std::filesystem::exists(AddonPath)) {
>>>>>>> Stashed changes
        firstInstall = true;
    }
    std::filesystem::create_directory(AddonPath);
    Settings::Load(SettingsPath);
    g_windowRenderer = std::make_unique<wvwfightanalysis::gui::WindowRenderer>();

<<<<<<< Updated upstream
    if (Settings::useNexusEscClose) {
        APIDefs->UI.RegisterCloseOnEscape("WvW Fight Analysis", &Settings::IsAddonWindowEnabled);
=======
    for (auto& mainWindow : Settings::windowManager.mainWindows) {
        if (mainWindow->useNexusEscClose) {
            APIDefs->UI.RegisterCloseOnEscape(
                mainWindow->windowId.c_str(),
                &mainWindow->isEnabled
            );
        }
    }

    for (auto& widgetWindow : Settings::windowManager.widgetWindows) {
        if (widgetWindow->useNexusEscClose) {
            APIDefs->UI.RegisterCloseOnEscape(
                widgetWindow->windowId.c_str(),
                &widgetWindow->isEnabled
            );
        }
    }

    if (Settings::windowManager.aggregateWindow && Settings::windowManager.aggregateWindow->useNexusEscClose) {
        APIDefs->UI.RegisterCloseOnEscape(
            Settings::windowManager.aggregateWindow->windowId.c_str(),
            &Settings::windowManager.aggregateWindow->isEnabled
        );
<<<<<<< Updated upstream
<<<<<<< Updated upstream
<<<<<<< Updated upstream
<<<<<<< Updated upstream
>>>>>>> Stashed changes
=======
>>>>>>> Stashed changes
=======
>>>>>>> Stashed changes
=======
>>>>>>> Stashed changes
=======
>>>>>>> Stashed changes
    }

    APIDefs->InputBinds.RegisterWithString(KB_WINDOW_TOGGLEVISIBLE, ProcessKeybinds, "(null)");
    APIDefs->InputBinds.RegisterWithString("KB_WIDGET_TOGGLEVISIBLE", ProcessKeybinds, "(null)");

    APIDefs->InputBinds.RegisterWithString("LOG_INDEX_UP", ProcessKeybinds, "(null)");
    APIDefs->InputBinds.RegisterWithString("LOG_INDEX_DOWN", ProcessKeybinds, "(null)");
    APIDefs->InputBinds.RegisterWithString("SHOW_SQUAD_PLAYERS_ONLY", ProcessKeybinds, "(null)");
    initMaps();
    directoryMonitorThread = std::thread(monitorDirectory, Settings::logHistorySize, Settings::pollIntervalMilliseconds);
    APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, "Addon loaded successfully.");
}

void AddonUnload()
{
    stopMonitoring = true;
<<<<<<< Updated upstream
<<<<<<< Updated upstream
<<<<<<< Updated upstream
<<<<<<< Updated upstream
<<<<<<< Updated upstream


    if (directoryMonitorThread.joinable())
    {
        directoryMonitorThread.join();
    }

    if (initialParsingThread.joinable())
    {
=======
    if (directoryMonitorThread.joinable()) {
        directoryMonitorThread.join();
    }
=======
    if (directoryMonitorThread.joinable()) {
        directoryMonitorThread.join();
    }
>>>>>>> Stashed changes
=======
    if (directoryMonitorThread.joinable()) {
        directoryMonitorThread.join();
    }
>>>>>>> Stashed changes
=======
    if (directoryMonitorThread.joinable()) {
        directoryMonitorThread.join();
    }
>>>>>>> Stashed changes
=======
    if (directoryMonitorThread.joinable()) {
        directoryMonitorThread.join();
    }
>>>>>>> Stashed changes
    if (initialParsingThread.joinable()) {
>>>>>>> Stashed changes
        initialParsingThread.join();
    }
    g_windowRenderer.reset();

<<<<<<< Updated upstream
<<<<<<< Updated upstream
<<<<<<< Updated upstream
<<<<<<< Updated upstream
<<<<<<< Updated upstream
    if (Settings::useNexusEscClose) {
        APIDefs->UI.DeregisterCloseOnEscape("WvW Fight Analysis");
=======
=======
>>>>>>> Stashed changes
=======
>>>>>>> Stashed changes
=======
>>>>>>> Stashed changes
=======
>>>>>>> Stashed changes
    for (auto& mainWindow : Settings::windowManager.mainWindows) {
        if (mainWindow->useNexusEscClose) {
            APIDefs->UI.DeregisterCloseOnEscape(mainWindow->windowId.c_str());
        }
    }

    for (auto& widgetWindow : Settings::windowManager.widgetWindows) {
        if (widgetWindow->useNexusEscClose) {
            APIDefs->UI.DeregisterCloseOnEscape(widgetWindow->windowId.c_str());
        }
    }

    if (Settings::windowManager.aggregateWindow && Settings::windowManager.aggregateWindow->useNexusEscClose) {
        APIDefs->UI.DeregisterCloseOnEscape(Settings::windowManager.aggregateWindow->windowId.c_str());
<<<<<<< Updated upstream
<<<<<<< Updated upstream
<<<<<<< Updated upstream
<<<<<<< Updated upstream
>>>>>>> Stashed changes
=======
>>>>>>> Stashed changes
=======
>>>>>>> Stashed changes
=======
>>>>>>> Stashed changes
=======
>>>>>>> Stashed changes
    }

    APIDefs->Renderer.Deregister(AddonRender);
    APIDefs->Renderer.Deregister(AddonOptions);
    APIDefs->InputBinds.Deregister("KB_WINDOW_TOGGLEVISIBLE");
    APIDefs->InputBinds.Deregister("KB_WIDGET_TOGGLEVISIBLE");
    APIDefs->InputBinds.Deregister("LOG_INDEX_UP");
    APIDefs->InputBinds.Deregister("LOG_INDEX_DOWN");
    APIDefs->InputBinds.Deregister("SHOW_SQUAD_PLAYERS_ONLY");
    APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, "Addon unloaded successfully.");
}

void ProcessKeybinds(const char* aIdentifier, bool aIsRelease)
{
    std::string str = aIdentifier;
    if (aIsRelease) return;

    if (str == "KB_WINDOW_TOGGLEVISIBLE")
    {
        Settings::IsAddonWindowEnabled = !Settings::IsAddonWindowEnabled;
        Settings::Save(SettingsPath);
    }
    else if (str == "KB_WIDGET_TOGGLEVISIBLE")
    {
        Settings::IsAddonWidgetEnabled = !Settings::IsAddonWidgetEnabled;
        Settings::Save(SettingsPath);
    }
    else if (str == "LOG_INDEX_DOWN") 
    {
        if (!parsedLogs.empty()) {
            if (currentLogIndex == 0) {
                currentLogIndex = static_cast<int>(parsedLogs.size()) - 1;
            }
            else {
                currentLogIndex--;
            }
        }
        else {
            APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
                ("Log Index: " + std::to_string(currentLogIndex)).c_str());
        }

    }
    else if (str == "LOG_INDEX_UP")
    {
        if (!parsedLogs.empty()) {
            currentLogIndex = (currentLogIndex + 1) % static_cast<int>(parsedLogs.size());
        }
        else {
            APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
                ("Log Index: " + std::to_string(currentLogIndex)).c_str());
        }
    }
    else if (str == "SHOW_SQUAD_PLAYERS_ONLY")
    {
        Settings::squadPlayersOnly = !Settings::squadPlayersOnly;
        Settings::Save(SettingsPath);
    }
}

void AddonRender()
{

<<<<<<< Updated upstream
<<<<<<< Updated upstream
<<<<<<< Updated upstream
<<<<<<< Updated upstream

    if (!NexusLink || !NexusLink->IsGameplay || !MumbleLink || MumbleLink->Context.IsMapOpen)
    {
        return;
    }

    if (
        MumbleLink->Context.MapType != Mumble::EMapType::WvW_EternalBattlegrounds &&
        MumbleLink->Context.MapType != Mumble::EMapType::WvW_BlueBorderlands &&
        MumbleLink->Context.MapType != Mumble::EMapType::WvW_GreenBorderlands &&
        MumbleLink->Context.MapType != Mumble::EMapType::WvW_RedBorderlands &&
        MumbleLink->Context.MapType != Mumble::EMapType::WvW_ObsidianSanctum &&
        MumbleLink->Context.MapType != Mumble::EMapType::WvW_EdgeOfTheMists &&
        MumbleLink->Context.MapType != Mumble::EMapType::WvW_Lounge
        ) {
        return;
    }

    if (MumbleLink->Context.IsInCombat && !Settings::showWindowInCombat)
    {
        return;
    }


    ratioBarSetup(hSelf);
    DrawAggregateStatsWindow(hSelf);
    RenderMainWindow(hSelf);

<<<<<<< Updated upstream
=======
=======
>>>>>>> Stashed changes
=======
>>>>>>> Stashed changes
=======
>>>>>>> Stashed changes
=======
>>>>>>> Stashed changes
    if (g_windowRenderer) {
        g_windowRenderer->RenderAllWindows(hSelf);
    }

<<<<<<< Updated upstream
<<<<<<< Updated upstream
<<<<<<< Updated upstream
<<<<<<< Updated upstream
>>>>>>> Stashed changes
=======
>>>>>>> Stashed changes
=======
>>>>>>> Stashed changes
=======
>>>>>>> Stashed changes
=======
>>>>>>> Stashed changes
}

void AddonOptions()
{
    ImGui::Text("WvW Fight Analysis Settings");
    if (ImGui::Checkbox("Window Enabled##WvWFightAnalysis", &Settings::IsAddonWindowEnabled))
    {
<<<<<<< Updated upstream
        Settings::Settings[IS_ADDON_WINDOW_VISIBLE] = Settings::IsAddonWindowEnabled;
        Settings::Save(SettingsPath);
=======
        // Global Settings
        if (ImGui::BeginTabItem("Global Settings"))
        {
            if (ImGui::InputText("Custom Log Path", Settings::LogDirectoryPathC, sizeof(Settings::LogDirectoryPathC))) {
                Settings::LogDirectoryPath = Settings::LogDirectoryPathC;
                Settings::Settings[CUSTOM_LOG_PATH] = Settings::LogDirectoryPath;
                Settings::Save(SettingsPath);
            }

            int tempLogHistorySize = static_cast<int>(Settings::logHistorySize);
            if (ImGui::InputInt("Log History Size", &tempLogHistorySize)) {
                Settings::logHistorySize = static_cast<size_t>(std::clamp(tempLogHistorySize, 1, 20));
                Settings::Settings[LOG_HISTORY_SIZE] = Settings::logHistorySize;
                Settings::Save(SettingsPath);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("How many parsed logs to keep.");
            }

            if (ImGui::InputInt("Team Player Threshold", &Settings::teamPlayerThreshold)) {
                Settings::teamPlayerThreshold = std::clamp(Settings::teamPlayerThreshold, 0, 100);
                Settings::Settings[TEAM_PLAYER_THRESHOLD] = Settings::teamPlayerThreshold;
                Settings::Save(SettingsPath);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Set a minimum number of team players required to render team.");
            }

            if (ImGui::Checkbox("Show Alert On Log Parse", &Settings::showNewParseAlert)) {
                Settings::Settings[SHOW_NEW_PARSE_ALERT] = Settings::showNewParseAlert;
                Settings::Save(SettingsPath);
            }

            if (ImGui::Checkbox("Enable Wine Compatibility Mode", &Settings::forceLinuxCompatibilityMode)) {
                Settings::Settings[FORCE_LINUX_COMPAT] = Settings::forceLinuxCompatibilityMode;
                Settings::Save(SettingsPath);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Wine/Proton doesn't support ReadDirectoryChangesW, use directory polling instead.");
            }

            if (Settings::forceLinuxCompatibilityMode) {
                int tempPollIntervalMilliseconds = static_cast<int>(Settings::pollIntervalMilliseconds);
                if (ImGui::InputInt("ms Polling Interval", &tempPollIntervalMilliseconds)) {
                    Settings::pollIntervalMilliseconds =
                        static_cast<size_t>(std::clamp(tempPollIntervalMilliseconds, 500, 10000));
                    Settings::Settings[POLL_INTERVAL_MILLISECONDS] = Settings::pollIntervalMilliseconds;
                    Settings::Save(SettingsPath);
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Polling Interval when using Wine compatibility mode.");
                }
            }

            if (ImGui::Checkbox("Enable Debug Logging", &Settings::debugStringsMode)) {
                Settings::Settings[DEBUG_STRINGS_MODE] = Settings::debugStringsMode;
                Settings::Save(SettingsPath);
            }


            bool enabled = !isRestartInProgress.load();
            if (!enabled) {
                ImVec4 disabledColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
                ImGui::PushStyleColor(ImGuiCol_Button, disabledColor);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, disabledColor);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, disabledColor);
            }

            if (ImGui::Button("Restart Directory Monitoring") && enabled) {
                isRestartInProgress.store(true);
                std::thread([]() {
                    stopMonitoring = true;
                    if (directoryMonitorThread.joinable()) {
                        directoryMonitorThread.join();
                    }
                    stopMonitoring = false;
                    directoryMonitorThread = std::thread(
                        monitorDirectory,
                        Settings::logHistorySize,
                        Settings::pollIntervalMilliseconds
                    );
                    isRestartInProgress.store(false);
                    }).detach();
            }

            if (!enabled) {
                ImGui::PopStyleColor(3);
            }

            ImGui::EndTabItem();
        }

        // Main Windows
        if (ImGui::BeginTabItem("Windows"))
        {
            ImGui::BeginGroup();
            if (ImGui::Button("Add Window")) {
                Settings::windowManager.AddMainWindow();
                Settings::Save(SettingsPath);
            }
            ImGui::SameLine();
            if (ImGui::Button("Remove Window") && Settings::windowManager.mainWindows.size() > 1) {
                Settings::windowManager.RemoveMainWindow();
                Settings::Save(SettingsPath);
            }
            ImGui::EndGroup();
            ImGui::Separator();

            int windowIndex = 0;
            for (auto& window : Settings::windowManager.mainWindows) {
                ImGui::PushID(windowIndex);

                if (ImGui::Checkbox("##Enabled", &window->isEnabled)) {
                    Settings::Save(SettingsPath);
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Enable or disable this main window.");
                }

                ImGui::SameLine();

                std::string displayName = window->windowName.empty() ?
                    "Main Window " + std::to_string(windowIndex + 1) : window->windowName;
                bool nodeOpen = ImGui::TreeNodeEx(
                    "##Node",
                    ImGuiTreeNodeFlags_AllowItemOverlap,
                    "%s",
                    displayName.c_str()
                );

                if (nodeOpen) {

                    if (ImGui::Checkbox("Use Nexus Esc to Close", &window->useNexusEscClose)) {
                        if (window->useNexusEscClose) {
                            RegisterWindowForNexusEsc(window.get(), "WvW Fight Analysis");
                        }
                        else {
                            UnregisterWindowFromNexusEsc(window.get(), "WvW Fight Analysis");
                        }
                        Settings::Save(SettingsPath);
                    }

                    if (ImGui::Checkbox("Hide In Combat", &window->hideInCombat)) {
                        Settings::Save(SettingsPath);
                    }
                    if (ImGui::Checkbox("Hide Out Of Combat", &window->hideOutOfCombat)) {
                        Settings::Save(SettingsPath);
                    }
                    if (ImGui::Checkbox("Lock Position", &window->disableMoving)) {
                        Settings::Save(SettingsPath);
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Disables moving & resizing.");
                    }
                    if (ImGui::Checkbox("Enable Mouse-Through", &window->disableClicking)) {
                        Settings::Save(SettingsPath);
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Window cannot be interacted with via mouse.");
                    }

                    ImGui::TreePop();
                }

                ImGui::PopID();
                windowIndex++;
            }

            ImGui::EndTabItem();
        }

        // Widget Windows
        if (ImGui::BeginTabItem("Widgets"))
        {
            ImGui::BeginGroup();
            if (ImGui::Button("Add Widget")) {
                Settings::windowManager.AddWidgetWindow();
                Settings::Save(SettingsPath);
            }
            ImGui::SameLine();
            if (ImGui::Button("Remove Widget") && Settings::windowManager.widgetWindows.size() > 1) {
                Settings::windowManager.RemoveWidgetWindow();
                Settings::Save(SettingsPath);
            }
            ImGui::EndGroup();
            ImGui::Separator();

            int windowIndex = 0;
            for (auto& window : Settings::windowManager.widgetWindows) {
                ImGui::PushID(windowIndex);

                if (ImGui::Checkbox("##Enabled", &window->isEnabled)) {
                    Settings::Save(SettingsPath);
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Enable or disable this widget window.");
                }

                ImGui::SameLine();

                std::string displayName = window->windowName.empty() ?
                    "Widget Window " + std::to_string(windowIndex + 1) : window->windowName;
                bool nodeOpen = ImGui::TreeNodeEx(
                    "##Node",
                    ImGuiTreeNodeFlags_AllowItemOverlap,
                    "%s",
                    displayName.c_str()
                );

                if (nodeOpen) {

                    if (ImGui::Checkbox("Use Nexus Esc to Close", &window->useNexusEscClose)) {
                        if (window->useNexusEscClose) {
                            RegisterWindowForNexusEsc(window.get(), "Team Ratio Bar");
                        }
                        else {
                            UnregisterWindowFromNexusEsc(window.get(), "Team Ratio Bar");
                        }
                        Settings::Save(SettingsPath);
                    }

                    if (ImGui::Checkbox("Hide In Combat", &window->hideInCombat)) {
                        Settings::Save(SettingsPath);
                    }
                    if (ImGui::Checkbox("Hide Out Of Combat", &window->hideOutOfCombat)) {
                        Settings::Save(SettingsPath);
                    }
                    if (ImGui::Checkbox("Lock Position", &window->disableMoving)) {
                        Settings::Save(SettingsPath);
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Disables moving & resizing.");
                    }
                    if (ImGui::Checkbox("Enable Mouse-Through", &window->disableClicking)) {
                        Settings::Save(SettingsPath);
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Window cannot be interacted with via mouse.");
                    }

                    ImGui::TreePop();
                }

                ImGui::PopID();
                windowIndex++;
            }

            ImGui::EndTabItem();
        }

        // Aggregate Window
        if (ImGui::BeginTabItem("Aggregate Window"))
        {
            auto& window = Settings::windowManager.aggregateWindow;

            if (ImGui::Checkbox("Enabled", &window->isEnabled)) {
                Settings::Save(SettingsPath);
            }

            if (ImGui::Checkbox("Use Nexus Esc to Close", &window->useNexusEscClose)) {
                if (window->useNexusEscClose) {
                    RegisterWindowForNexusEsc(window.get(), "Aggregate Stats");
                }
                else {
                    UnregisterWindowFromNexusEsc(window.get(), "Aggregate Stats");
                }
                Settings::Save(SettingsPath);
            }

            if (ImGui::Checkbox("Hide In Combat", &window->hideInCombat)) {
                Settings::Save(SettingsPath);
            }
            if (ImGui::Checkbox("Hide Out Of Combat", &window->hideOutOfCombat)) {
                Settings::Save(SettingsPath);
            }
            if (ImGui::Checkbox("Hide When Empty", &window->hideWhenEmpty)) {
                Settings::Save(SettingsPath);
            }
            if (ImGui::Checkbox("Lock Position", &window->disableMoving)) {
                Settings::Save(SettingsPath);
            }
            if (ImGui::Checkbox("Enable Mouse-Through", &window->disableClicking)) {
                Settings::Save(SettingsPath);
            }

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
>>>>>>> Stashed changes
    }
    if (ImGui::Checkbox("Widget Enabled##WvWFightAnalysis", &Settings::IsAddonWidgetEnabled))
    {
        Settings::Settings[IS_ADDON_WIDGET_VISIBLE] = Settings::IsAddonWidgetEnabled;
        Settings::Save(SettingsPath);
    }
    if (ImGui::Checkbox("Aggregate Stats Enabled##WvWFightAnalysis", &Settings::IsAddonAggWindowEnabled))
    {
        Settings::Settings[IS_ADDON_AGG_WINDOW_VISIBLE] = Settings::IsAddonAggWindowEnabled;
        Settings::Save(SettingsPath);
    }
    if (ImGui::Checkbox("Hide Aggregate Stats Window When Empty##WvWFightAnalysis", &Settings::hideAggWhenEmpty))
    {
        Settings::Settings[HIDE_AGG_WINDOW_WHEN_EMPTY] = Settings::IsAddonAggWindowEnabled;
        Settings::Save(SettingsPath);
    }
    if (ImGui::Checkbox("Visible In Combat##WvWFightAnalysis", &Settings::showWindowInCombat))
    {
        Settings::Settings[IS_WINDOW_VISIBLE_IN_COMBAT] = Settings::showWindowInCombat;
        Settings::Save(SettingsPath);
    }
    if (ImGui::Checkbox("Show Alert On Log Parse##WvWFightAnalysis", &Settings::showNewParseAlert))
    {
        Settings::Settings[SHOW_NEW_PARSE_ALERT] = Settings::showNewParseAlert;
        Settings::Save(SettingsPath);
    }
    if (ImGui::Checkbox("Use Nexus Esc to Close##WvWFightAnalysis", &Settings::useNexusEscClose))
    {
        Settings::Settings[USE_NEXUS_ESC_CLOSE] = Settings::useNexusEscClose;
        Settings::Save(SettingsPath);

        if (Settings::useNexusEscClose)
        {
            APIDefs->UI.RegisterCloseOnEscape("WvW Fight Analysis", &Settings::IsAddonWindowEnabled);
        }
        else
        {
            APIDefs->UI.DeregisterCloseOnEscape("WvW Fight Analysis");
        }
    }
    if (ImGui::Checkbox("Lock Window & Widget Position##WvWFightAnalysis", &Settings::disableMovingWindow))
    {
        Settings::Settings[DISABLE_MOVING_WINDOW] = Settings::disableMovingWindow;
        Settings::Save(SettingsPath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Disables moving & resizing.");
        ImGui::EndTooltip();
    }
    if (ImGui::Checkbox("Enable Mouse-Through##WvWFightAnalysis", &Settings::disableClickingWindow))
    {
        Settings::Settings[DISABLE_CLICKING_WINDOW] = Settings::disableClickingWindow;
        Settings::Save(SettingsPath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Window cannot be interacted with via mouse.");
        ImGui::EndTooltip();
    }
    if (ImGui::Checkbox("Enable Wine Compatibility Mode##WvWFightAnalysis", &Settings::forceLinuxCompatibilityMode))
    {
        Settings::Settings[FORCE_LINUX_COMPAT] = Settings::forceLinuxCompatibilityMode;
        Settings::Save(SettingsPath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Wine / Proton doesn't support ReadDirectoryChangesW, use directory polling instead.");
        ImGui::EndTooltip();
    }
    int tempPollIntervalMilliseconds = static_cast<int>(Settings::pollIntervalMilliseconds);
    if (ImGui::InputInt("ms Polling Interval##WvWFightAnalysis", &tempPollIntervalMilliseconds))
    {
        Settings::pollIntervalMilliseconds = static_cast<size_t>(std::clamp(tempPollIntervalMilliseconds, 500, 10000));
        Settings::Settings[POLL_INTERVAL_MILLISECONDS] = Settings::pollIntervalMilliseconds;
        Settings::Save(SettingsPath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Polling Interval when using Wine compatibility mode.");
        ImGui::EndTooltip();
    }
    if (ImGui::InputInt("Team Player Threshold##WvWFightAnalysis", &Settings::teamPlayerThreshold))
    {
        Settings::teamPlayerThreshold = std::clamp(
            Settings::teamPlayerThreshold, 0,100
        );
        Settings::Settings[TEAM_PLAYER_THRESHOLD] = Settings::teamPlayerThreshold;
        Settings::Save(SettingsPath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Set a minimum amount of team players required to render team.");
        ImGui::EndTooltip();
    }
    int tempLogHistorySize = static_cast<int>(Settings::logHistorySize);
    if (ImGui::InputInt("Log History Size##WvWFightAnalysis", &tempLogHistorySize))
    {
        Settings::logHistorySize = static_cast<size_t>(std::clamp(tempLogHistorySize, 1, 20));
        Settings::Settings[LOG_HISTORY_SIZE] = Settings::logHistorySize;
        Settings::Save(SettingsPath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("How many parsed logs to keep.");
        ImGui::EndTooltip();
    }
    if (ImGui::InputText("Custom Log Path##WvWFightAnalysis", Settings::LogDirectoryPathC, sizeof(Settings::LogDirectoryPathC)))
    {
        Settings::LogDirectoryPath = Settings::LogDirectoryPathC;
        Settings::Settings[CUSTOM_LOG_PATH] = Settings::LogDirectoryPath;
        Settings::Save(SettingsPath);
    }

    bool enabled = !isRestartInProgress.load();

    if (!enabled) {
        ImVec4 disabledColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, disabledColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, disabledColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, disabledColor);
    }

    bool buttonClicked = ImGui::Button("Restart Directory Monitoring");

    if (!enabled) {
        ImGui::PopStyleColor(3);
    }

    if (enabled && buttonClicked) {
        isRestartInProgress.store(true);
        std::thread([]() {
            stopMonitoring = true;
            if (directoryMonitorThread.joinable())
            {
                directoryMonitorThread.join();
            }
            stopMonitoring = false;
            directoryMonitorThread = std::thread(monitorDirectory, Settings::logHistorySize, Settings::pollIntervalMilliseconds);
            isRestartInProgress.store(false);
            }).detach();
    }

}

extern "C" __declspec(dllexport) AddonDefinition * GetAddonDef()
{
    AddonDef.Signature = -996748;
    AddonDef.APIVersion = NEXUS_API_VERSION;
    AddonDef.Name = ADDON_NAME;
    AddonDef.Version.Major = 1;
<<<<<<< Updated upstream
    AddonDef.Version.Minor = 0;
    AddonDef.Version.Build = 4;
    AddonDef.Version.Revision = 0;
=======
    AddonDef.Version.Minor = 1;
    AddonDef.Version.Build = 0;
    AddonDef.Version.Revision = 5;
<<<<<<< Updated upstream
<<<<<<< Updated upstream
<<<<<<< Updated upstream
<<<<<<< Updated upstream
>>>>>>> Stashed changes
=======
>>>>>>> Stashed changes
=======
>>>>>>> Stashed changes
=======
>>>>>>> Stashed changes
=======
>>>>>>> Stashed changes
    AddonDef.Author = "Unreal";
    AddonDef.Description = "WvW log analysis tool.";
    AddonDef.Load = AddonLoad;
    AddonDef.Unload = AddonUnload;
    AddonDef.Flags = EAddonFlags_None;
    AddonDef.Provider = EUpdateProvider_GitHub;
    AddonDef.UpdateLink = "https://github.com/jake-greygoose/WvW-Fight-Analysis-Addon";
    return &AddonDef;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH: hSelf = hModule; break;
    case DLL_PROCESS_DETACH: break;
    case DLL_THREAD_ATTACH: break;
    case DLL_THREAD_DETACH: break;
    }
    return TRUE;
}