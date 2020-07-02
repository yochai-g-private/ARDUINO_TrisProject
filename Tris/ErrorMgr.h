#pragma once

struct ErrorMgr
{
    static void Report(const char* reason);
    static void Load();
    static void Store();
    static void Clean();

    static void AddWebServices(AsyncWebServer& server);


};
