#pragma once

#include "Tris.h"

namespace NYG 
{
    enum NightlyMode
    {
        NM_DISABLED,    __min_NightlyMode__ = NM_DISABLED,
        NM_AIR,
        NM_ALL,         __max_NightlyMode__ = NM_ALL,
    };

    struct Settings
    {
        char    name[16];
        uint8_t version;

        struct States
        {
            bool        manual;
            
            struct
            {
                NightlyMode mode;

                uint16_t    down, //  time in per-day-minutes format
                            up;   //  time in per-day-minutes format, up==SUNRISE means at sun rise
            }   nightly;
            
            struct
            {
                bool        on;
                uint8_t     minutes_after_sun_rise,
                            duration_minutes;
            }   sun_protect;

            bool        DST;
        }   states;

        struct Timings
        {
            struct Direction {
                double  all,
                        air,
                        sun;
            } up, down;
        }   timings;

        static void Load();
        static void Store(const NYG::Settings& temp);

        static void AddWebServices(AsyncWebServer& server);

        enum { SUNRISE = 0xFFFF };

        static void WriteApplicationInfoToLog()
        {
            _LOGGER << "Application: " << defaults.name << NL;
            _LOGGER << "Version    : " << defaults.version << NL;
        }
    private:

        static Settings defaults;

    };

    extern Settings settings;

};

