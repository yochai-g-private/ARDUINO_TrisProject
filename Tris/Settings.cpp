#include "Settings.h"

#include "EepromIO.h"


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

        ErrorMgr::Clean();

        LOGGER << "Settings written" << NL;
    }
    else
    {
        ErrorMgr::Load();
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

        OnSettingsChanged();
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
            false,                          // manual;
            { NM_ALL, 0, 0xFFFF },          // nightly;
            { true,  60, 60 },              // sun_protect;
            true,                           // DST;
        },

        //  Timings
            {
                //all air sun
                { 29.5,  4, 18 },                 // up
                { 27.5, 22, 11 },                 // down
            }

    };

    Settings settings;
};