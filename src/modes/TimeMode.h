#pragma once

#include "DisplayMode.h"
#include "../Utils.h"

class TimeMode : public DisplayMode {
public:
    using DisplayMode::DisplayMode;

    void update() override {
        if (millis() - webServer.getLastCheckDateTime() > webServer.getDateCheckInterval()) {
            webServer.setLastCheckDateTime(millis());

            // Get user-friendly format from settings (fallback to "HH:mm")
            String userFormat = settings.getString("timeFormat").length() > 0 ? settings.getString("timeFormat") : "HH:mm";

            // Convert to strftime-compatible format
            String strftimeFormat = convertToStrftime(userFormat);
            String result = renderTime(strftimeFormat);

            // Write to display if it changed
            if (result != webServer.getWrittenString()) {
                display.writeString(result, MAX_RPM);
                webServer.setWrittenString(result);
            }
        }
    }

private:
    String renderTime(const String &format) {
        char buf[64];
        time_t now = time(nullptr);
        struct tm *timeinfo = localtime(&now);

        strftime(buf, sizeof(buf), format.c_str(), timeinfo);

        return trimToModuleCount(String(buf), display.getNumModules());
    }
};
