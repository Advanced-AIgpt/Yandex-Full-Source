#pragma once

#include <util/generic/fwd.h>
#include <util/stream/fwd.h>

namespace NGranet {

class TGrammar;

THashMap<TString, TSet<TString>> LoadSynonyms(const TString& pathMain, const TString& pathFixlist);

void PrintFormSynonyms(const TGrammar& grammar, const TString& formName, const THashMap<TString, TSet<TString>>& synonyms, IOutputStream* log);

}
