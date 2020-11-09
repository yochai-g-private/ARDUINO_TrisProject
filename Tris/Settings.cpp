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

	bool store_settings = true;

    if (memcmp(defaults.name, settings.name, sizeof(defaults.name)))
    {
        settings = defaults;
    }
	else if (settings.version != defaults.version)
	{
		switch (settings.version)
		{
			case 1: settings.internet_time = defaults.internet_time;
					break;
		}
	}
	else
    {
		store_settings = false;
        ErrorMgr::Load();
        LOGGER << "Settings OK" << NL;
    }

	if (store_settings)
	{
		EepromOutput settings_output;
		settings_output << settings;

		ErrorMgr::Clean();

		LOGGER << "Settings written" << NL;
	}

    gbl_DST = settings.internet_time.dst;
}
//------------------------------------------------------
void  Settings::Store(const NYG::Settings& temp)
{
    if (memcmp(&settings, &temp, sizeof(temp)))
    {
        settings = temp;
        gbl_DST = settings.internet_time.dst;

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
		VERSION,                            // version

        // States
        {
            false,                          // manual;
            { NM_ALL, 0, 0xFFFF },          // nightly;
            { true,  30, 85 },              // sun_protect;
            true,                           // DST;
        },

        //  Timings
        {
            //all air sun
            { 29.5,  4, 17 },                 // up
            { 27.5, 22, 11 },                 // down
        }

    };

    Settings settings;
};