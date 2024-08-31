#pragma once

#include "common.h"
#include "library/cpp/langs/langs.h"
#include <alice/nlu/libs/fst/fst_base.h>

class TParser {
public:
    TParser(const TConfigFst& fstConfig);
    TString Parse(const TString& text);

private:
    TString MakeResultString(const TVector<NAlice::TEntity>& entities, const TString& normalizedText);

private:
    TString FstName;
    TString EntityTypeName;
    ELanguage Language;
    THolder<NAlice::TFstBase> Fst;
};
