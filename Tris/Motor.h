#pragma once

struct Motor
{
    static void Initialize();
    static void TestRelays();

    static void Schedule();

    //static void SetState(TrisState state);
    //static void SetPower(bool on);
    //static void Stop(bool leave_it_in_unknown_state = false);

    static void OnLoop();

    static void AddWebServices(AsyncWebServer& server);

    static void PowerOff();

    static void TogglePower();
};