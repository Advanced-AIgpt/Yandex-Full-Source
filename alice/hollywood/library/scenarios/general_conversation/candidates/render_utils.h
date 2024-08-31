#pragma once

#include <alice/hollywood/library/base_scenario/fwd.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <util/generic/fwd.h>

namespace NAlice::NHollywood::NGeneralConversation {

void AddSuggest(const TString& actionId, const TString& renderedPhrase, const TString& analyticsType,
                bool forceGcResponse, TResponseBodyBuilder& bodyBuilder);
void AddSearchSuggest(const TString& renderedPhrase, TResponseBodyBuilder& bodyBuilder);
void AddShowView(NAlice::TRTLogger& logger, TResponseBodyBuilder& bodyBuilder);
void AddFrontalLedImage(const TVector<TString>& imageUrls, TResponseBodyBuilder* responseBodyBuilder);
void AddScledEyes(TResponseBodyBuilder* responseBodyBuilder);
void AddAction(const TString& frameName, TResponseBodyBuilder& bodyBuilder);
void AddPush(TResponseBodyBuilder& bodyBuilder, const TString& title, const TString& text,
             const TString& link, const TString& imageUrl, const TString& pushId);
void AddListenDirectiveWithCallback(int silenceTimeoutMs, TResponseBodyBuilder* bodyBuilder);

} // namespace NAlice::NHollywood::NGeneralConversation
