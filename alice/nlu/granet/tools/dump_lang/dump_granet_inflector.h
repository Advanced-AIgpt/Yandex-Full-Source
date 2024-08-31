#pragma once

#include <library/cpp/langmask/langmask.h>
#include <util/stream/output.h>

void DumpGranetInflector(TWtringBuf phrase, const TLangMask& langs, TStringBuf grams, bool isVerbose,
    IOutputStream* log);
