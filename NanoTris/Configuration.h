#pragma once

#include "Tris.h"

struct Configuration : public StdConfig
{
    Configuration();

    struct Data
    {
        struct States
        {
            bool on;
            bool nightly;
            bool sun_protect;
            bool DST;
        }   states;

        struct Timing
        {
            struct Direction {
                double  all,
                        air,
                        sun;
            } up, down;
        }   timings;
    }

protected:

    bool load(EepromInput& in);
    void store(EepromOutput& out);
    void show()		const;

};

extern Configuration cfg;
