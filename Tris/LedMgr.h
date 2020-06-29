#pragma once

struct LedMgr
{
    static void Test();

    static void OnStateChanged(State prev);

    static void OnLoop();
};