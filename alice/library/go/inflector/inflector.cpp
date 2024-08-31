#pragma once

#include "inflector.h"
#include <kernel/inflectorlib/phrase/simple/simple.h>
#include <util/charset/wide.h>

char* Inflect(char* text, char* casus, char** err)
{
    NInfl::TSimpleInflector inflector("ru");
    try {
        return strdup(WideToUTF8(inflector.Inflect(UTF8ToWide(TString(text)), TString(casus))).data());
    }
    catch (const std::exception& ex) {
        *err = strdup(ex.what());
        return text;
    }
}
