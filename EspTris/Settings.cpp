#include <EEPROM.h>
#include "EepromIO.h"

#include "Settings.h"

using namespace NYG;

namespace NYG
{
    extern bool gbl_DST;
};

void  Settings::Load()
{
    EepromInput settings_input;
    settings_input >> settings;

    if (memcmp(defaults.name, settings.name, sizeof(defaults.name)) || settings.version != defaults.version)
    {
        settings = defaults;
        EepromOutput settings_output;
        settings_output << settings;

        LOGGER << "Settings written" << NL;
    }
    else
    {
        LOGGER << "Settings OK" << NL;
    }

    gbl_DST = settings.states.DST;
}
//------------------------------------------------------
void  Settings::Store(const NYG::Settings& temp)
{
    if (memcmp(&settings, &temp, sizeof(temp)))
    {
        settings = temp;
        gbl_DST = settings.states.DST;

        EepromOutput settings_output;
        settings_output << temp;
    }
}
//------------------------------------------------------

namespace NYG
{
    Settings Settings::defaults =
    {
        "TRIS",                             // name
        1,                                  // version

        // States
        {
            true,                           // manual;
            { NM_DISABLED, 0, 0xFFFF },     // nightly;
            { false, 60, 60 },              // sun_protect;
            false,                          // DST;
        },

        //  Timings
            {
                //all air sun
                { 29,  5, 15 },                 // up
                { 27, 23, 12 },                 // down
            }

    };

    Settings settings;
};