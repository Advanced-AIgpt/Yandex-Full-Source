#pragma once

#include <library/cpp/langmask/langmask.h>
#include <kernel/lemmer/core/language.h>
#include <kernel/lemmer/core/lemmer.h>
#include <kernel/lemmer/dictlib/tgrammar_processing.h>
#include <util/stream/output.h>
#include <util/string/join.h>

void DumpNormalizer(TWtringBuf line, const TLangMask& langs, IOutputStream* log, const TString& indent = "");
