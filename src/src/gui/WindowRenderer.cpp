#include "gui/WindowRenderer.h"
#include "mumble/Mumble.h"
#include "nexus/Nexus.h"


namespace wvwfightanalysis::gui {

    WindowRenderer::WindowRenderer() {
        // Constructor is empty since window instances are created as member variables
    }

    bool WindowRenderer::IsInWvWMap() const {
        if (!MumbleLink) return false;

        return MumbleLink->Context.MapType == Mumble::EMapType::WvW_EternalBattlegrounds ||
            MumbleLink->Context.MapType == Mumble::EMapType::WvW_BlueBorderlands ||
            MumbleLink->Context.MapType == Mumble::EMapType::WvW_GreenBorderlands ||
            MumbleLink->Context.MapType == Mumble::EMapType::WvW_RedBorderlands ||
            MumbleLink->Context.MapType == Mumble::EMapType::WvW_ObsidianSanctum ||
            MumbleLink->Context.MapType == Mumble::EMapType::WvW_EdgeOfTheMists ||
            MumbleLink->Context.MapType == Mumble::EMapType::WvW_Lounge;
    }

    void WindowRenderer::UpdateGameState() {
        if (!NexusLink || !MumbleLink) {
            m_isInWvW = false;
            m_isInCombat = false;
            m_isMapOpen = false;
            return;
        }

        m_isInWvW = IsInWvWMap() && NexusLink->IsGameplay;
        m_isInCombat = MumbleLink->Context.IsInCombat;
        m_isMapOpen = MumbleLink->Context.IsMapOpen;
    }

    bool WindowRenderer::ShouldRenderWindow(const BaseWindowSettings* settings) const {
        if (!settings || !settings->isEnabled) return false;

        if (m_isInCombat && settings->hideInCombat) return false;
        if (!m_isInCombat && settings->hideOutOfCombat) return false;

        return true;
    }

    void WindowRenderer::RenderAllWindows(HINSTANCE hSelf) {
        // Update state before rendering
        UpdateGameState();

        // Don't render if we're not in WvW or the map is open
        if (!m_isInWvW || m_isMapOpen) {
            return;
        }

        // Render main windows using Settings::windowManager
        for (const auto& window : Settings::windowManager.mainWindows) {
            if (!ShouldRenderWindow(window.get())) continue;

            ImGui::SetNextWindowSizeConstraints(ImVec2(100, 100), ImVec2(FLT_MAX, FLT_MAX));
            m_mainWindow.Render(hSelf, window.get());
        }

        // Render widget windows
        for (const auto& widget : Settings::windowManager.widgetWindows) {
            if (!ShouldRenderWindow(widget.get())) continue;

            m_widgetWindow.Render(hSelf, widget.get());
        }

        // Render aggregate window
        if (Settings::windowManager.aggregateWindow &&
            ShouldRenderWindow(Settings::windowManager.aggregateWindow.get())) {
            ImGui::SetNextWindowSizeConstraints(ImVec2(100, 100), ImVec2(FLT_MAX, FLT_MAX));
            m_aggregateWindow.Render(hSelf, Settings::windowManager.aggregateWindow.get());
        }
    }

} // namespace wvwfightanalysis::gui