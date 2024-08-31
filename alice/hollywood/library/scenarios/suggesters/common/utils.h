#pragma once

#include <alice/hollywood/library/response/response_builder.h>

#include <alice/megamind/protos/common/frame.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>
#include <alice/protos/data/language/language.pb.h>

#include <util/generic/hash_set.h>
#include <util/generic/maybe.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NAlice::NHollywood {

TMaybe<TStringBuf> TryGetFlagValue(const TStringBuf flag, const TStringBuf key);

TSemanticFrame InitCallbackFrameEffect(const TSemanticFrame& semanticFrame, const TString& frameName,
                                       const THashSet<TString>& copySlots);

TMaybe<TString> GetContentTypeByGenre(const TMaybe<TString>& genre);

template <typename TStringContainer>
inline TFrameNluHint BuildFrameNluHint(const TStringContainer& phrases) {
    TFrameNluHint nluHint;

    for (const TString& phrase : phrases) {
        TNluPhrase& nluPhrase = *nluHint.AddInstances();
        nluPhrase.SetLanguage(ELang::L_RUS);
        nluPhrase.SetPhrase(phrase);
    }

    return nluHint;
}

void AddTypeTextSuggest(const TString& text, const TString& actionId, TResponseBodyBuilder& bodyBuilder);

} // namespace NAlice::NHollywood
