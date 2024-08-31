#pragma once

#include <alice/hollywood/library/scenarios/news/proto/news.pb.h>

#include <alice/library/json/json.h>

namespace NAlice::NHollywood {

const NJson::TJsonValue& GetSlot(const NJson::TJsonValue& bassResponse, const TString name);
const NJson::TJsonValue& GetSlotValue(const NJson::TJsonValue& bassResponse, const TString name);
NJson::TJsonValue GetBassResponse(const TNewsState& state);
void AppendBassResponse(NJson::TJsonValue& bassResponse, const NJson::TJsonValue& add);
const NJson::TJsonValue& GetNewsSlot(const NJson::TJsonValue& bassResponse);
int GetNewsCount(const NJson::TJsonValue& newsSlot);

} // namespace NAlice::NHollywood
