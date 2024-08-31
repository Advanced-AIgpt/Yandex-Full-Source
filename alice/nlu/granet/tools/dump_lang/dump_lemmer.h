#pragma once

#include <library/cpp/langmask/langmask.h>
#include <kernel/lemmer/core/language.h>
#include <kernel/lemmer/core/lemmer.h>
#include <kernel/lemmer/dictlib/tgrammar_processing.h>
#include <util/stream/output.h>
#include <util/string/join.h>

template <>
inline void Out<TGrammarVector>(IOutputStream& out, const TGrammarVector& grammars) {
    out << '{' << JoinSeq(", ", grammars) << '}';
}

void DumpLemmer(TWtringBuf word, const TLangMask& langs, bool isVerbose, IOutputStream* log, const TString& indent = "");
void Dump(const TWLemmaArray& lemmas, bool isVerbose, IOutputStream* log, const TString& indent = "");
void Dump(const TYandexLemma& lemma, bool isVerbose, IOutputStream* log, const TString& indent = "");
void Dump(const TYandexWordform& form, IOutputStream* log, const TString& indent = "");
