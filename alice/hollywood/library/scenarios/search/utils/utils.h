#pragma once

#include <alice/hollywood/library/frame/callback.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/library/json/json.h>

#include <alice/protos/data/language/language.pb.h>

#include <util/string/builder.h>

namespace NAlice::NHollywood::NSearch {

TString RemoveHighlight(TStringBuf str);

bool AddDivCardImage(const TString& src, ui16 w, ui16 h, NJson::TJsonValue& card);

TString CreateAvatarIdImageSrc(const NJson::TJsonValue& cardData, ui16 w, ui16 h);

TString GetHostName(const NJson::TJsonValue& snippet);
TString ParseHostName(const TString& url);

TString ForceString(const NJson::TJsonValue& value);

bool IsNumber(const NJson::TJsonValue& value);

long long ForceInteger(const NJson::TJsonValue& value);

void AppendFormattedTime(ui32 value, TStringBuf* names, TStringBuilder& dst);

TString FormatTimeDifference(i32 diff, TStringBuf tld);

TString TryRoundFloat(const TString& value);

TString JoinListFact(const TString& text, const TVector<TString>& items, bool isOrdered, bool isTts = false);

template<unsigned long PhrasesSize>
TFrameNluHint CreateNluHint(const TString& nluFrameName, const std::array<TStringBuf, PhrasesSize>& phrases) {
    TFrameNluHint nluHint;
    nluHint.SetFrameName(nluFrameName);
    for (const TStringBuf& phrase: phrases) {
        TNluPhrase& nluPhrase = *nluHint.AddInstances();
        nluPhrase.SetLanguage(ELang::L_RUS);
        nluPhrase.SetPhrase(phrase.data(), phrase.size());
    }

    return nluHint;
}

template<unsigned long PhrasesSize>
void AddFrameActionWithHint(const TString& actionId, TSemanticFrame&& frame, TResponseBodyBuilder& bodyBuilder,
                            const std::array<TStringBuf, PhrasesSize>& phrases)
{
    NScenarios::TFrameAction action;
    *action.MutableNluHint() = CreateNluHint(frame.GetName(), phrases);
    *action.MutableCallback() = ToCallback(frame);
    bodyBuilder.AddAction(actionId, std::move(action));
}

} // namespace NAlice::NHollywood
