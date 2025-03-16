#pragma once
#include <Windows.h>
#include "settings/Settings.h"
#include "shared/Shared.h"

namespace wvwfightanalysis::gui {

    class AggregateWindow {
    public:
        void Render(HINSTANCE hSelf, AggregateWindowSettings* settings);

    private:
        void RenderTeamSection(
            const std::string& teamName,
            const TeamAggregateStats& teamAgg,
            const AggregateWindowSettings* settings,
            HINSTANCE hSelf,
            float fontSize
        );
        void RenderSettingsPopup(AggregateWindowSettings* settings);
        void RenderStyleSelector(AggregateWindowSettings* settings);
        void RenderDisplaySettings(AggregateWindowSettings* settings);
    };

} // namespace