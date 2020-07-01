#pragma once

#include <ESPAsyncWebServer.h>
#include "AsyncWebServerEx.h""

void InitializeWebServices();

String GetElementValue(const char* field, const String& value);
Html::Element& CreateField(const char* field, const String& value, Html::TextGeneratorCtx& ctx);

#define _IndentAttribute(mul, add)  Html::StyleAttribute::Indent(mul * (ctx.depth + add))
#define _BackgroundImageAttribute()  Html::StyleAttribute::BackgroundImage("Tris_1.png")
#define FieldIndentAttribute  _IndentAttribute(4, 1)
#define RootIndentAttribute   _IndentAttribute(2, 0)

#define SetRoot(name, url)    \
    Html::Element& root = *new Html::Paragraph();\
    const char* element_name = #name ":";\
    if (ctx.depth)    {\
        Html::h1& h1 = *new Html::h1();\
        h1.AddAttribute(RootIndentAttribute);\
        root.AddChild(h1);\
        h1.AddChild(*new Html::Anchor(element_name, url)); }\
    else { root.AddChild(*new Html::h1(element_name)); }

#define SendHtml(func_id)  \
    Html::TextGeneratorCtx ctx;\
    Html::Element& e = get##func_id(ctx);\
    SendElement( e, *request );\
    delete &e

