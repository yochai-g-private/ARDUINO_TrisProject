#include "Tris.h"
#include "ErrorMgr.h"
#include "Motor.h"
#include "WebServices.h"

#include "EepromIO.h"

//------------------------------------------------------
struct Err
{
    DstTime now;
    char    reason[128];
};
//------------------------------------------------------
struct ErrorsRegistry
{
    bool    set_state;
    Err     errors[4];
};

static ErrorsRegistry   registry = { 0 };

//------------------------------------------------------
void ErrorMgr::Report(const char* reason)
{
    if(registry.set_state)
        gbl_State = Error;

    Motor::PowerOff();
    
    LOGGER << "ERROR: " << reason << NL;

    DstTime now = DstTime::Now();

    if (strequal(reason, registry.errors[0].reason) && ((now - registry.errors[0].now) < SECONDS_PER_HOUR))
        return;

    for (int idx = countof(registry.errors) - 1; idx >= 1; idx--)
        registry.errors[idx] = registry.errors[idx - 1];

    registry.errors[0].now = now;
    strncpy(registry.errors[0].reason, reason, countof(registry.errors[0].reason) - 1);

    Store();
}
//------------------------------------------------------
void ErrorMgr::Store()
{
    EepromOutput eeprom;

    eeprom.SetPosition(sizeof(Settings));
    eeprom << registry;
}
//------------------------------------------------------
void ErrorMgr::Load()
{
    EepromInput eeprom;

    eeprom.SetPosition(sizeof(Settings));
    eeprom >> registry;
}
//------------------------------------------------------
void ErrorMgr::Clean()
{
    memset(&registry.errors, 0, sizeof(registry.errors));
    Store();
}
//------------------------------------------------------
static Html::Element& getErrors(Html::TextGeneratorCtx& ctx)
{
    SetRoot(Errors, "/errors");

    ctx.depth++;
    Html::Paragraph& p = *new Html::Paragraph();
    p.AddAttribute(FieldIndentAttribute);
    root.AddChild(p);

    for (int idx = 0; idx < countof(registry.errors) && *registry.errors[idx].reason; idx++)
    {
        p.AddChild(*new Html::h3(String(registry.errors[idx].now.ToText()) + "\t - \t" + registry.errors[idx].reason));
    }

    ctx.depth--;

    root.AddChild(*new Html::h3(GetElementValue("set_state", GetYesNo(registry.set_state))));

    return _root;
}
//------------------------------------------------------
static void WS_Errors(AsyncWebServerRequest *request)
{
    bool b;
    
    if (GetBoolParam(*request, "set_state", b) && (registry.set_state != b))
    {
        registry.set_state = b;
        ErrorMgr::Store();
    }

    SendHtml(Errors);
}
//------------------------------------------------------
static void WS_Clean(AsyncWebServerRequest *request)
{
    ErrorMgr::Clean();
    SendHtml(Errors);
}
//------------------------------------------------------
void ErrorMgr::AddWebServices(AsyncWebServer& server)
{
    server.on("/errors", HTTP_GET, WS_Errors);
    server.on("/errors/clean", HTTP_GET, WS_Clean);

}
//------------------------------------------------------
