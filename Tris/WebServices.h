#pragma once

#include <ESPAsyncWebServer.h>
#include "Html.h""

void InitializeWebServices();

class AsyncWebServerRequest;
void SendElement(Html::Element& e, AsyncWebServerRequest& request);

extern const String TextHtmlContentType;
//extern bool WebServiceFound;


