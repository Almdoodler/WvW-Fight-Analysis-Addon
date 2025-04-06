#pragma once
#include <Windows.h>
#include "settings/Settings.h"
#include "shared/Shared.h"
#include "windows/MainWindow.h"
#include "windows/WidgetWindow.h"
#include "windows/AggregateWindow.h"

namespace wvwfightanalysis::gui {

    class WindowRenderer {
    public:
        WindowRenderer();
        ~WindowRenderer() = default;

        // Main render function
        void RenderAllWindows(HINSTANCE hSelf);

    private:
        // Window instances
        MainWindow m_mainWindow;
        WidgetWindow m_widgetWindow;
        AggregateWindow m_aggregateWindow;

        // Rendering helpers
        bool ShouldRenderWindow(const BaseWindowSettings* settings) const;
        bool IsInWvWMap() const;
        void UpdateGameState();

        // State tracking
        bool m_isInCombat = false;
        bool m_isInWvW = false;
        bool m_isMapOpen = false;
    };

} // namespace wvwfightanalysis::gui