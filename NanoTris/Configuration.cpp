#include "Configuration.h"
#include "EepromIO.h"

static const Configuration::Timing  default_timings = {
    //all air sun
    { 29,  5, 15 },      // up
    { 27, 23, 12 },      // down
};

Configuration::Configuration() : StdConfig("Tris", 0)
{
    timings = default_timings;
}

bool Configuration::load(EepromInput& in)
{
    if (!StdConfig::load(in))
        return false;

    in.Read(timings);
}
//--------------------------------------------------------
void Configuration::store(EepromOutput& out)
{
    StdConfig::store(out);
    out.Write(timings);
}
//--------------------------------------------------------
static void show_timing_direction(const char* name, const Configuration::Timing::Direction& d)
{
    _LOGGER << "  " << name << ":" << NL;
    _LOGGER << "    " << "all: " << d.all << NL;
    _LOGGER << "    " << "air: " << d.air << NL;
    _LOGGER << "    " << "sun: " << d.sun << NL;
}
//--------------------------------------------------------
void Configuration::show()		const
{
    _LOGGER << "Timing: " << NL;
    show_timing_direction("DOWN", timings.down);
    show_timing_direction("UP",   timings.up);
}
//--------------------------------------------------------
