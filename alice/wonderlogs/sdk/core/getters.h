#pragma once

#include <alice/library/client/protos/client_info.pb.h>
#include <alice/megamind/protos/analytics/megamind_analytics_info.pb.h>
#include <alice/megamind/protos/common/frame.pb.h>
#include <alice/megamind/protos/scenarios/analytics_info.pb.h>
#include <alice/megamind/protos/speechkit/response.pb.h>

#include <library/cpp/json/writer/json_value.h>

#include <util/generic/maybe.h>

namespace NAlice::NWonderSdk {

namespace NImpl {

struct TAction {
    TMaybe<TString> CardId;
    TMaybe<TString> IntentName;
    TMaybe<TString> ActionName;
};

TAction ParseAction(const NJson::TJsonValue& action, bool onlyLogId);
TVector<TAction> ParseElement(const NJson::TJsonValue& element, bool onlyLogId);
TMaybe<TString> RestoreFormName(const NMegamind::TMegamindAnalyticsInfo& analyticsInfo,
                                const NJson::TJsonValue& callbackArgs);

} // namespace NImpl

struct TCard {
    TString Text;
    TString Type;
    TVector<TMaybe<TString>> Actions;
    TMaybe<TString> CardId;
    TMaybe<TString> IntentName;
    NJson::TJsonValue ToJson() const;
};

TMaybe<TString> GetIntent(const NMegamind::TMegamindAnalyticsInfo& analyticsInfo);
TMaybe<TString> GetProductScenarioName(const NMegamind::TMegamindAnalyticsInfo& analyticsInfo);
TMaybe<TString> GetMusicAnswerType(const NMegamind::TMegamindAnalyticsInfo& analyticsInfo);
TMaybe<TString> GetMusicGenre(const NMegamind::TMegamindAnalyticsInfo& analyticsInfo);
TMaybe<TString> GetFiltersGenre(const NMegamind::TMegamindAnalyticsInfo& analyticsInfo);
bool SmartHomeUser(const NMegamind::TMegamindAnalyticsInfo& analyticsInfo);
TString GetApp(const TClientInfoProto& app);
TString GetPlatform(const TClientInfoProto& app);
TString GetVersion(const TClientInfoProto& app);
TMaybe<double> GetSoundLevel(const TMaybe<double>& soundLevel, const TStringBuf appId);
TMaybe<TString> GetPath(const NMegamind::TMegamindAnalyticsInfo& analyticsInfo, const NJson::TJsonValue& callbackArgs);
bool FormChanged(const NMegamind::TMegamindAnalyticsInfo& analyticsInfo, const NJson::TJsonValue& callbackArgs);
TVector<TSemanticFrame::TSlot> GetSlots(const NMegamind::TMegamindAnalyticsInfo& analyticsInfo);
std::variant<std::monostate, TString, NJson::TJsonValue> GetSlotValue(const TSemanticFrame::TSlot& slot);
TMaybe<NDialogovo::TSkill> GetSkillInfo(const NMegamind::TMegamindAnalyticsInfo& analyticsInfo,
                                        const TVector<TSemanticFrame::TSlot>& slots);
TString NormalizeScenarioName(const TMaybe<TString>& path, const bool containsDirectives,
                              const TVector<TSpeechKitResponseProto::TResponse::TCard>& cards,
                              const TMaybe<TString>& skillId, const NJson::TJsonValue& callbackArgs,
                              const TVector<TSemanticFrame::TSlot>& slots);
TMaybe<TCard> ParseCard(const TSpeechKitResponseProto::TResponse::TCard& card);
TVector<TCard> ParseCards(const TVector<TSpeechKitResponseProto::TResponse::TCard>& cards);

} // namespace NAlice::NWonderSdk
