#pragma once

#include <ESPAsyncWebServer.h>
#include "AsyncWebServerEx.h""
#include "Html.h"

void InitializeWebServices();

String GetElementValue(const char* field, const String& value);
Html::Element& CreateField(const char* field, const String& value, Html::TextGeneratorCtx& ctx);

void SendInstantHtml(AsyncWebServerRequest& request, Html::Element& e);

#define _IndentAttribute(mul, add)  Html::StyleAttribute::Indent(mul * (ctx.depth + add))
#define _BackgroundImageAttribute()  Html::StyleAttribute::BackgroundImage("Tris_1.png")
#define FieldIndentAttribute  _IndentAttribute(2, 1)
#define RootIndentAttribute   _IndentAttribute(1, 0)

#define SetRoot(name, url)    \
    Html::Element& _root = *new Html::Paragraph();\
    Html::Element* proot;\
    const char* element_name = #name ":";\
    if (ctx.depth)    {\
        Html::h1& h1 = *new Html::h1();\
        h1.AddAttribute(RootIndentAttribute);\
        _root.AddChild(h1);\
        h1.AddChild(*new Html::Anchor(element_name, url)); \
        proot = &_root; }\
    else { \
        _root.AddChild(*new Html::h1(element_name)); \
        Html::Div & div = *new Html::Div("english");\
        _root.AddChild(div);\
        proot = &div; }\
    Html::Element& root = *proot


#define SendHtml(func_id)  \
    Html::TextGeneratorCtx ctx;\
    Html::Element& e = get##func_id(ctx);\
    SendInstantHtml(*request, e)

