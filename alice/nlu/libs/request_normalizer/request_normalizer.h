#pragma once

#include "request_tokenizer.h"
#include <alice/nlu/libs/fst/fst_normalizer.h>

#include <library/cpp/langs/langs.h>

#include <util/generic/map.h>

namespace NNlu {

class TRequestNormalizer : public TNonCopyable {
public:
    static TString Normalize(ELanguage lang, TStringBuf text);

    // TODO(samoylovboris) Remove
    static void WarmUpSingleton();

private:
    TRequestNormalizer();

    void AddDenormalizer(ELanguage lang, TStringBuf langDir);
    TString DoNormalize(ELanguage lang, TStringBuf text) const;
    static TString Preprocess(TString text);
    static TString Postprocess(TString text);

    Y_DECLARE_SINGLETON_FRIEND()

private:
    // Normalizer and Denormalizer are made in NLG. Original propose of these decoders is:
    // - Normalizer (not used here) - makes "speakable" text from "typewritten" form with digits and abbreviations:
    //                                "$12" -> "twelve dollars"
    // - Denormalizer - converts "speakable" text into "typewritten" form (for search):
    //                  "twelve" -> "12"
    TMap<ELanguage, NAlice::TFstDecoder> Denormalizers;
};

} // namespace NNlu
