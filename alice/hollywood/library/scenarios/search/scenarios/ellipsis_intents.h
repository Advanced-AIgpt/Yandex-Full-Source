#pragma once

#include <alice/hollywood/library/scenarios/search/context/context.h>

namespace NAlice::NHollywood::NSearch {

void AddFactoidSrcSuggest(TSearchContext& ctx, const NJson::TJsonValue& data);
void AddSerpVoiceButton(TSearchContext& ctx);
void AddSerpConfirmationButton(TSearchContext& ctx);
void AddShowOnMapButton(TSearchContext& ctx, const TString& mapUrl);
void AddFactoidCallButton(TSearchContext& ctx, const TString& phoneUri);
void AddPushButton(TSearchContext& ctx, const TString& nluHintFrame = "alice.push_notification");

bool CheckEllipsisIntents(TSearchContext& ctx);
bool HasEllipsisFrames(const TScenarioRunRequestWrapper& request);

} // namespace NAlice::NHollywood
