#include "getters.h"

#include <alice/library/json/json.h>
#include <alice/megamind/protos/analytics/analytics_info.pb.h>
#include <alice/megamind/protos/analytics/user_info.pb.h>
#include <alice/megamind/protos/analytics/scenarios/dialogovo/dialogovo.pb.h>
#include <alice/megamind/protos/analytics/scenarios/music/music.pb.h>
#include <alice/megamind/protos/analytics/scenarios/vins/vins.pb.h>
#include <alice/megamind/protos/common/iot.pb.h>
#include <alice/megamind/protos/property/iot_profile.pb.h>
#include <alice/megamind/protos/property/property.pb.h>
#include <alice/megamind/protos/scenarios/user_info.pb.h>

#include <dict/dictutil/str.h>

#include <google/protobuf/struct.pb.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/json/fast_sax/parser.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <library/cpp/string_utils/url/url.h>

#include <util/generic/cast.h>
#include <util/generic/hash_set.h>
#include <util/string/strip.h>

using namespace NAlice::NWonderSdk;

namespace {

const TString HOLLYWOOD_MUSIC = "HollywoodMusic";
const TString VINS = "Vins";
const TString DIALOGOVO = "Dialogovo";
// const TStringBuf SERP_INTENT = "serp";
constexpr TStringBuf DIV_CARD = "div_card";

const TString DEFAULT_SCENARIO_NAME = "EMPTY";

const char GENRE_SEP = ':';
const auto GENRE_PREFIX = TString("genre") + GENRE_SEP;

constexpr TStringBuf SMART_SPEAKER_DEVICES_PREFIX = "devices.types.smart_speaker";
const THashSet<TStringBuf> BANNED_SMART_HOME_TYPES = {
    "devices.types.hub",                               // RC
    "devices.types.media_device.dongle.yandex.module", // Modul
};
constexpr TStringBuf TV_DEVICE_TYPE = "devices.types.media_device.tv";
constexpr TStringBuf QUASAR_SKILL_ID = "Q";

bool SmartHomeAnalyticsTypeSkillId(const TStringBuf type, const TStringBuf skillId) {
    return !type.StartsWith(SMART_SPEAKER_DEVICES_PREFIX) && !BANNED_SMART_HOME_TYPES.contains(type) &&
           !(type == TV_DEVICE_TYPE && skillId == QUASAR_SKILL_ID);
}

const TString COM_YANDEX_LAUNCHER = "com.yandex.launcher";
const TString RU_YANDEX_MOBILE_NAVIGATOR = "ru.yandex.mobile.navigator";

const THashMap<TString, TString> APPS_MAP = {{"ru.yandex.searchplugin", "search_app_prod"},
                                             {"ru.yandex.mobile", "search_app_prod"},
                                             {"ru.yandex.searchplugin.beta", "search_app_beta"},
                                             {"ru.yandex.mobile.search", "browser_prod"},
                                             {"ru.yandex.mobile.search.ipad", "browser_prod"},
                                             {"winsearchbar", "stroka"},
                                             {"com.yandex.browser", "browser_prod"},
                                             {"com.yandex.browser.alpha", "browser_alpha"},
                                             {"com.yandex.browser.beta", "browser_beta"},
                                             {"ru.yandex.yandexnavi", "navigator"},
                                             {RU_YANDEX_MOBILE_NAVIGATOR, "navigator"},
                                             {COM_YANDEX_LAUNCHER, "launcher"},
                                             {"ru.yandex.quasar.services", "quasar"},
                                             {"yandex.auto", "auto"},
                                             {"ru.yandex.autolauncher", "auto"},
                                             {"ru.yandex.iosdk.elariwatch", "elariwatch"},
                                             {"aliced", "small_smart_speakers"},
                                             {"YaBro", "yabro_prod"},
                                             {"YaBro.beta", "yabro_beta"},
                                             {"ru.yandex.quasar.app", "quasar"},
                                             {"yandex.auto.old", "auto_old"},
                                             {"ru.yandex.mobile.music", "music_app_prod"},
                                             {"ru.yandex.music", "music_app_prod"},
                                             {"com.yandex.tv.alice", "tv"},
                                             {"ru.yandex.taximeter", "taximeter"},
                                             {"ru.yandex.yandexmaps", "yandexmaps_prod"},
                                             {"ru.yandex.traffic", "yandexmaps_prod"},
                                             {"ru.yandex.yandexmaps.debug", "yandexmaps_dev"},
                                             {"ru.yandex.yandexmaps.pr", "yandexmaps_dev"},
                                             {"ru.yandex.traffic.inhouse", "yandexmaps_dev"},
                                             {"ru.yandex.traffic.sandbox", "yandexmaps_dev"},
                                             {"com.yandex.alice", "alice_app"},
                                             {"ru.yandex.centaur", "centaur"},
                                             {"ru.yandex.sdg.taxi.inhouse", "sdc"},
                                             {"com.yandex.iot", "iot_app"},
                                             {"legatus", "legatus"}};

inline bool Turkish(const TStringBuf lang) {
    return (lang == "tr" || lang == "tr-TR");
}

inline bool TurkishNavi(const TStringBuf appId, const TStringBuf lang) {
    // Just ru.yandex.mobile.navigator
    // https://a.yandex-team.ru/svn/trunk/arcadia/alice/analytics/operations/dialog/sessions/usage_fields.py?blame=true&rev=r9623262#L116
    return appId == RU_YANDEX_MOBILE_NAVIGATOR && Turkish(lang);
}

void ParseElementImpl(const NJson::TJsonValue& element, TVector<NImpl::TAction>& actions, const bool onlyLogId) {
    if (element.IsMap()) {
        TVector<TStringBuf> keys;
        for (const auto& [k, _] : element.GetMap()) {
            keys.push_back(k);
        }
        // There's need of order for consistency
        Sort(keys);
        for (const auto& k : keys) {
            const auto& v = element.GetMap().at(k);
            if (k == "action" && !v.IsNull()) {
                actions.emplace_back(NImpl::ParseAction(v, onlyLogId));
            } else {
                ParseElementImpl(v, actions, onlyLogId);
            }
        }
    } else if (element.IsArray()) {
        for (const auto& subElement : element.GetArray()) {
            ParseElementImpl(subElement, actions, onlyLogId);
        }
    }
}

} // namespace

namespace NAlice::NWonderSdk {

namespace NImpl {

TAction ParseAction(const NJson::TJsonValue& action, const bool onlyLogId) {
    const TStringBuf url = action["url"].GetString();
    TStringBuf query;
    {
        TStringBuf host;
        SplitUrlToHostAndPath(url, host, query);
    }

    TCgiParameters cgis;
    constexpr TStringBuf PREFIX_TO_DELETE = "//?";
    // I don't know how to make it better
    cgis.ScanAddAll(query.SubStr(PREFIX_TO_DELETE.Size()));
    constexpr TStringBuf DIRECTIVES_CGI = "directives";
    if (onlyLogId || !cgis.Has(DIRECTIVES_CGI)) {
        TMaybe<TString> actionName;
        if (const auto* logId = action.GetValueByPath("log_id"); logId && logId->IsString()) {
            actionName = logId->GetString();
        }
        return {.ActionName = std::move(actionName)};
    }

    {
        NJson::TJsonValue directives;
        if (NJson::ReadJsonTree(UrlUnescapeRet(cgis.Get(DIRECTIVES_CGI)), &directives)) {
            for (const auto& directive : directives.GetArray()) {
                if (directive["name"].GetString() == "on_card_action") {
                    const auto& payload = directive["payload"];
                    TAction action;
                    if (const auto* cardIdPtr = payload.GetValueByPath("card_id");
                        cardIdPtr && cardIdPtr->IsString()) {
                        action.CardId = cardIdPtr->GetString();
                    }
                    if (const auto* intentNamePtr = payload.GetValueByPath("intent_name");
                        intentNamePtr && intentNamePtr->IsString()) {
                        action.IntentName = intentNamePtr->GetString();
                    }
                    if (const auto* actionNamePtr = payload.GetValueByPath("action_name");
                        actionNamePtr && actionNamePtr->IsString()) {
                        action.ActionName = actionNamePtr->GetString();
                    }
                    if (!action.ActionName || action.ActionName->Empty()) {
                        if (const auto* caseNamePtr = payload.GetValueByPath("case_name");
                            caseNamePtr && caseNamePtr->IsString()) {
                            action.ActionName = caseNamePtr->GetString();
                        }
                    }
                    return action;
                }
            }
        }
    }
    return {};
}

TVector<TAction> ParseElement(const NJson::TJsonValue& element, const bool onlyLogId) {
    TVector<TAction> actions;
    ParseElementImpl(element, actions, onlyLogId);
    return actions;
}

TMaybe<TString> RestoreFormName(const NMegamind::TMegamindAnalyticsInfo& analyticsInfo,
                                const NJson::TJsonValue& callbackArgs) {
    // Request for a form update. No guarantee that this intent will be called
    // https://a.yandex-team.ru/svn/trunk/arcadia/alice/analytics/operations/dialog/sessions/usage_fields.py?rev=r9665641#L92
    if (const auto* formUpdateName = callbackArgs.GetValueByPath("form_update.name");
        formUpdateName && formUpdateName->IsString()) {
        return formUpdateName->GetString();
    }
    return GetPath(analyticsInfo, callbackArgs);
}

} // namespace NImpl

NJson::TJsonValue TCard::ToJson() const {
    NJson::TJsonValue json;
    constexpr TStringBuf TEXT = "text";
    constexpr TStringBuf TYPE = "type";
    constexpr TStringBuf ACTIONS = "actions";
    constexpr TStringBuf CARD_ID = "card_id";
    constexpr TStringBuf INTENT_NAME = "intent_name";

    json[TEXT] = Text;
    json[TYPE] = Type;
    if (Type != DIV_CARD) {
        return json;
    }

    {
        auto& jsonActions = json[ACTIONS];
        jsonActions = NJson::JSON_ARRAY;
        for (const auto& action : Actions) {
            if (action) {
                jsonActions.AppendValue(*action);
            } else {
                jsonActions.AppendValue(NJson::JSON_NULL);
            }
        }
    }
    json[CARD_ID] = NJson::JSON_NULL;
    if (CardId) {
        json[CARD_ID] = *CardId;
    }
    json[INTENT_NAME] = NJson::JSON_NULL;
    if (CardId) {
        json[INTENT_NAME] = *IntentName;
    }
    return json;
}

TMaybe<TString> GetIntent(const NMegamind::TMegamindAnalyticsInfo& analyticsInfo) {
    if (auto scenarioAnalyticsInfoIt =
            analyticsInfo.GetAnalyticsInfo().find(analyticsInfo.GetWinnerScenario().GetName());
        scenarioAnalyticsInfoIt != analyticsInfo.GetAnalyticsInfo().end()) {
        if (scenarioAnalyticsInfoIt->second.HasScenarioAnalyticsInfo()) {
            return scenarioAnalyticsInfoIt->second.GetScenarioAnalyticsInfo().GetIntent();
        }
    }
    return {};
}

TMaybe<TString> GetProductScenarioName(const NMegamind::TMegamindAnalyticsInfo& analyticsInfo) {
    if (auto scenarioAnalyticsInfoIt =
            analyticsInfo.GetAnalyticsInfo().find(analyticsInfo.GetWinnerScenario().GetName());
        scenarioAnalyticsInfoIt != analyticsInfo.GetAnalyticsInfo().end()) {
        if (scenarioAnalyticsInfoIt->second.HasScenarioAnalyticsInfo()) {
            return scenarioAnalyticsInfoIt->second.GetScenarioAnalyticsInfo().GetProductScenarioName();
        }
    }
    return {};
}

TMaybe<TString> GetMusicAnswerType(const NMegamind::TMegamindAnalyticsInfo& analyticsInfo) {
    if (auto scenarioAnalyticsInfoIt =
            analyticsInfo.GetAnalyticsInfo().find(analyticsInfo.GetWinnerScenario().GetName());
        scenarioAnalyticsInfoIt != analyticsInfo.GetAnalyticsInfo().end()) {
        if (scenarioAnalyticsInfoIt->second.HasScenarioAnalyticsInfo()) {
            const auto& scenarioAnalyticsInfo = scenarioAnalyticsInfoIt->second.GetScenarioAnalyticsInfo();
            for (const auto& event : scenarioAnalyticsInfo.GetEvents()) {
                if (event.GetEventCase() == NAlice::NScenarios::TAnalyticsInfo::TEvent::kMusicEvent) {
                    auto* value = event.GetMusicEvent().EAnswerType_descriptor()->FindValueByNumber(
                        event.GetMusicEvent().GetAnswerType());
                    if (value) {
                        return value->name();
                    }
                }
            }
        }
    }
    return {};
}

TMaybe<TString> GetMusicGenre(const NMegamind::TMegamindAnalyticsInfo& analyticsInfo) {
    if (auto scenarioAnalyticsInfoIt = analyticsInfo.GetAnalyticsInfo().find(HOLLYWOOD_MUSIC);
        scenarioAnalyticsInfoIt != analyticsInfo.GetAnalyticsInfo().end()) {
        if (scenarioAnalyticsInfoIt->second.HasScenarioAnalyticsInfo()) {
            const auto& scenarioAnalyticsInfo = scenarioAnalyticsInfoIt->second.GetScenarioAnalyticsInfo();
            if (scenarioAnalyticsInfo.ObjectsSize() > 0) {
                if (!scenarioAnalyticsInfo.GetObjects(0).GetFirstTrack().GetGenre().Empty()) {
                    return scenarioAnalyticsInfo.GetObjects(0).GetFirstTrack().GetGenre();
                }
            }
        }
    }
    return {};
}

TMaybe<TString> GetFiltersGenre(const NMegamind::TMegamindAnalyticsInfo& analyticsInfo) {
    if (auto scenarioAnalyticsInfoIt = analyticsInfo.GetAnalyticsInfo().find(HOLLYWOOD_MUSIC);
        scenarioAnalyticsInfoIt != analyticsInfo.GetAnalyticsInfo().end()) {
        if (scenarioAnalyticsInfoIt->second.HasScenarioAnalyticsInfo()) {
            const auto& scenarioAnalyticsInfo = scenarioAnalyticsInfoIt->second.GetScenarioAnalyticsInfo();
            for (const auto& event : scenarioAnalyticsInfo.GetEvents()) {
                if (event.GetEventCase() == NAlice::NScenarios::TAnalyticsInfo::TEvent::kMusicEvent &&
                    event.GetMusicEvent().GetAnswerType() ==
                        NAlice::NScenarios::TAnalyticsInfo::TEvent::TMusicEvent::Filters) {
                    const TStringBuf id = event.GetMusicEvent().GetId();
                    if (id.StartsWith(GENRE_PREFIX)) {
                        TStringBuf genre = id.SubString(GENRE_PREFIX.Size(), id.Size() - GENRE_PREFIX.Size());
                        if (genre.find(GENRE_SEP) == TString::npos) {
                            return TString{genre};
                        }
                    }
                }
            }
        }
    }
    return {};
}

bool SmartHomeUser(const NMegamind::TMegamindAnalyticsInfo& analyticsInfo) {
    if (analyticsInfo.HasIoTUserInfo()) {
        for (const auto& device : analyticsInfo.GetIoTUserInfo().GetDevices()) {
            if (!device.GetAnalyticsType().Empty()) {
                if (SmartHomeAnalyticsTypeSkillId(device.GetAnalyticsType(), device.GetSkillId())) {
                    return true;
                }
            }
        }
    } else if (!analyticsInfo.GetUsersInfo().empty()) {
        for (const auto& [_, usersInfo] : analyticsInfo.GetUsersInfo()) {
            for (const auto& property : usersInfo.GetScenarioUserInfo().GetProperties()) {
                for (const auto& device : property.GetIotProfile().GetDevices()) {
                    if (!device.GetType().Empty()) {
                        // There is no skill_id in property's devices
                        // https://a.yandex-team.ru/svn/trunk/arcadia/alice/analytics/operations/dialog/sessions/usage_fields.py?rev=r9623262#L834
                        // https://a.yandex-team.ru/svn/trunk/arcadia/alice/megamind/protos/property/iot_profile.proto?rev=r7795416#L21-32
                        if (SmartHomeAnalyticsTypeSkillId(device.GetType(), /* skillId= */ "")) {
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

TString GetApp(const TClientInfoProto& appInfo) {
    if (appInfo.GetAppId() == COM_YANDEX_LAUNCHER && appInfo.GetDeviceManufacturer() == "Yandex") {
        // https://a.yandex-team.ru/svn/trunk/arcadia/alice/analytics/operations/dialog/sessions/usage_fields.py?blame=true&rev=r9623262#L101
        return "yandex_phone";
    }
    TString app = "other";
    if (auto* appPtr = APPS_MAP.FindPtr(appInfo.GetAppId())) {
        app = *appPtr;
    }
    if (app == "navigator" && Turkish(appInfo.GetLang())) {
        // https://a.yandex-team.ru/svn/trunk/arcadia/alice/analytics/operations/dialog/sessions/usage_fields.py?blame=true&rev=r9623262#L104
        app = "tr_navigator";
    }
    return app;
}

TString GetPlatform(const TClientInfoProto& appInfo) {
    // Weird
    if (TurkishNavi(appInfo.GetAppId(), appInfo.GetLang()) && !appInfo.HasPlatform()) {
        // https://a.yandex-team.ru/svn/trunk/arcadia/alice/analytics/operations/dialog/sessions/usage_fields.py?blame=true&rev=r9623262#L118
        return "iphone";
    }
    // Very weird
    if (!appInfo.HasPlatform() || appInfo.GetPlatform().Empty()) {
        // https://a.yandex-team.ru/svn/trunk/arcadia/alice/analytics/operations/dialog/sessions/usage_fields.py?blame=true&rev=r9623262#L119
        return "android";
    }
    return appInfo.GetPlatform();
}

TString GetVersion(const TClientInfoProto& appInfo) {
    // Check HasPlatform yes
    // https://a.yandex-team.ru/svn/trunk/arcadia/alice/analytics/operations/dialog/sessions/usage_fields.py?blame=true&rev=r9623262#L125
    if (TurkishNavi(appInfo.GetAppId(), appInfo.GetLang()) && !appInfo.HasPlatform()) {
        // flex69
        // https://a.yandex-team.ru/svn/trunk/arcadia/alice/analytics/operations/dialog/sessions/usage_fields.py?blame=true&rev=r9623262#L126
        return "400";
    }
    return StripString(appInfo.GetAppVersion());
}

TMaybe<double> GetSoundLevel(const TMaybe<double>& soundLevel, const TStringBuf appId) {
    if (!soundLevel) {
        return {};
    }
    auto val = *soundLevel;
    if (auto* app = APPS_MAP.FindPtr(appId); app && *app == "tv") {
        // Originally 0 to 100 for TVs, 0 to 10 for everything else
        // https://a.yandex-team.ru/svn/trunk/arcadia/alice/analytics/operations/dialog/sessions/usage_fields.py?rev=r9623262#L741
        val /= 10;
    }
    return val;
}

TMaybe<TString> GetPath(const NMegamind::TMegamindAnalyticsInfo& analyticsInfo,
                        const NJson::TJsonValue& callbackArgs) {
    if (auto* suggestPathPtr = callbackArgs.GetValueByPath("suggest_block.form_update.name");
        suggestPathPtr && suggestPathPtr->IsString()) {
        return suggestPathPtr->GetString();
    }
    TMaybe<TString> path = analyticsInfo.GetWinnerScenario().GetName();
    if (analyticsInfo.GetWinnerScenario().GetName() == VINS) {
        path = GetIntent(analyticsInfo);
        auto scenarioAnalyticsInfoIt = analyticsInfo.GetAnalyticsInfo().find(VINS);
        if (scenarioAnalyticsInfoIt != analyticsInfo.GetAnalyticsInfo().end()) {
            for (const auto& object : scenarioAnalyticsInfoIt->second.GetScenarioAnalyticsInfo().GetObjects()) {
                if (object.HasVinsGcMeta()) {
                    if (!object.GetVinsGcMeta().GetIntent().Empty()) {
                        return object.GetVinsGcMeta().GetIntent();
                    }
                    return path;
                }
            }
        }
    }
    return path;
}

bool FormChanged(const NMegamind::TMegamindAnalyticsInfo& analyticsInfo, const NJson::TJsonValue& callbackArgs) {
    const auto restoredPath = NImpl::RestoreFormName(analyticsInfo, callbackArgs);
    const auto path = GetPath(analyticsInfo, callbackArgs);
    if (restoredPath.Defined() != path.Defined()) {
        return true;
    }
    if (!restoredPath.Defined() && !path.Defined()) {
        return false;
    }
    return *restoredPath != *path;
}

TVector<TSemanticFrame::TSlot> GetSlots(const NMegamind::TMegamindAnalyticsInfo& analyticsInfo) {
    if (analyticsInfo.GetWinnerScenario().GetName() == VINS) {
        const auto& slots = analyticsInfo.GetAnalyticsInfo()
                                .at(analyticsInfo.GetWinnerScenario().GetName())
                                .GetSemanticFrame()
                                .GetSlots();
        return TVector<TSemanticFrame::TSlot>(slots.begin(), slots.end());
    }
    return {};
}

std::variant<std::monostate, TString, NJson::TJsonValue> GetSlotValue(const TSemanticFrame::TSlot& slot) {
    switch (slot.GetTypedValue().GetValueCase()) {
        case TSlotValue::kString: {
            {
                NJson::TJsonValue potentialJson;
                if (NJson::ReadJsonTree(slot.GetTypedValue().GetString(), &potentialJson)) {
                    return potentialJson;
                }
            }
            return slot.GetTypedValue().GetString();
            break;
        }
        case TSlotValue::VALUE_NOT_SET: {
        }
    }
    return NJson::JSON_NULL;
}

TMaybe<NDialogovo::TSkill> GetSkillInfo(const NMegamind::TMegamindAnalyticsInfo& analyticsInfo,
                                        const TVector<TSemanticFrame::TSlot>& slots) {
    if (auto scenarioAnalyticsInfoIt = analyticsInfo.GetAnalyticsInfo().find(DIALOGOVO);
        analyticsInfo.GetWinnerScenario().GetName() == DIALOGOVO &&
        scenarioAnalyticsInfoIt != analyticsInfo.GetAnalyticsInfo().end()) {
        for (const auto& object : scenarioAnalyticsInfoIt->second.GetScenarioAnalyticsInfo().GetObjects()) {
            if (object.HasSkill()) {
                return object.GetSkill();
            }
        }
        return NDialogovo::TSkill();
    }
    for (const auto& slot : slots) {
        if (slot.GetName() == "skill_id") {
            const auto slotValue = GetSlotValue(slot);
            // Where other cases
            // https://a.yandex-team.ru/svn/trunk/arcadia/alice/analytics/operations/dialog/sessions/usage_fields.py?blame=true&rev=r9682043#L318
            if (std::holds_alternative<TString>(slotValue)) {
                const auto& skillId = std::get<TString>(slotValue);
                // Different
                // https://a.yandex-team.ru/svn/trunk/arcadia/alice/analytics/operations/dialog/sessions/usage_fields.py?blame=true&rev=r9682043#L319-321
                if (skillId == "null") {
                    continue;
                }
                NDialogovo::TSkill skill;
                skill.SetId(skillId);
                return skill;
            }
        }
    }
    return {};
}

// TString NormalizeScenarioName(const TMaybe<TString>& path, const bool containsDirectives,
//                               const TVector<TSpeechKitResponseProto::TResponse::TCard>& cards,
//                               const TMaybe<TString>& skillId, const NJson::TJsonValue& callbackArgs,
//                               const TVector<TSemanticFrame::TSlot>& slots) {
//     if (!path) {
//         return DEFAULT_SCENARIO_NAME;
//     }
//     TString resPath = *path;

//     if (resPath.StartsWith("personal_assistant.scenarios.search")) {
//         ReplaceAll(resPath, "search_internet", "search");
//         // Possibly bug according to the comment
//         //
//         https://a.yandex-team.ru/svn/trunk/arcadia/alice/analytics/operations/dialog/sessions/usage_fields.py?blame=true&rev=r9682043#L175-176
//         if (!resPath.EndsWith(SERP_INTENT) || resPath.EndsWith("show_on_map")) {
//             for (const auto& slot : slots) {
//                 const auto slotValue = GetSlotValue(slot);
//                 const auto hasSlotValue = !holds_alternative<std::monostate>(slotValue);
//                 if (slot.GetName() == "nav_url" && hasSlotValue) {
//                     ReplaceAll(resPath, "scenarios.search", "scenarios.search_nav_url");
//                 } else if (slot.GetName() == "search_results" && hasSlotValue) {
//                     auto postfix = SERP_INTENT;
//                     // What about other cases?
//                     if (holds_alternative<NJson::TJsonValue>(slotValue)) {
//                         const auto& jsonSlotValue = get<NJson::TJsonValue>(slotValue);
//                         if (jsonSlotValue.IsMap()) {
//                             for (const auto& [k, _] : jsonSlotValue.GetMap()) {
//                                 if (k != SERP_INTENT) {
//                                     postfix = k == "nav" ? "nav_url" : k;
//                                 }
//                             }
//                         }
//                     }
//                     break;
//                 }
//             }
//         }
//     } else if (resPath.Contains("external_skill")) {
//         // No idea what this id means
//         if (skillId && *skillId == "bd7c3799-5947-41d0-b3d3-4a35de977111") {
//             ReplaceAll(resPath, "external_skill", "external_skill_gc");
//         }
//     } else if (resPath.StartsWith("personal_assistant.scenarios.market") &&
//                !resPath.Contains("scenarios.market_beru")) {
//         const auto slotBlue = [](const TStringBuf value) { return value == "BLUE" || value == "\"BLUE\""; };
//         constexpr TStringBuf CHOICE_MARKET_TYPE = "choice_market_type";
//         const char* SCENARIOS_MARKET = "scenarios.market";
//         constexpr TStringBuf SCENARIOS_MARKET_BERU = "scenarios.market_beru";
//         bool choiceMarketTypeAndBlue = false;

//         for (const auto& slot : slots) {
//             const auto slotValue = GetSlotValue(slot);
//             const auto stringSlotValue = holds_alternative<TString>(slotValue);
//             const TStringBuf stringValue = get<TString>(slotValue);
//             if (slot.GetName() == CHOICE_MARKET_TYPE && stringSlotValue && slotBlue(stringValue)) {
//                 choiceMarketTypeAndBlue = true;
//             }
//         }
//         // VA-1092
//         auto endsWithMarket = false;
//         {
//             const auto* formUpdateName = callbackArgs.GetValueByPath("suggest_block.form_update.name");
//             endsWithMarket = formUpdateName && formUpdateName->IsString() &&
//                              formUpdateName->GetString().EndsWith(".market__market");
//         }

//         if (const auto* slotsPtr = callbackArgs.GetValueByPath("suggest_block.form_update.slots");
//             resPath.Contains(SCENARIOS_MARKET_BERU) && endsWithMarket && slotsPtr && slotsPtr->IsArray()) {
//             for (const auto& slot : slotsPtr->GetArray()) {
//                 constexpr TStringBuf VALUE = "value";
//                 if (slot["name"] == CHOICE_MARKET_TYPE && slot.Has(VALUE) && slot[VALUE].IsString() &&
//                     slotBlue(slot[VALUE].GetString())) {
//                     choiceMarketTypeAndBlue = true;
//                 }
//             }
//         }
//         if (choiceMarketTypeAndBlue) {
//             ReplaceAll(resPath, SCENARIOS_MARKET, SCENARIOS_MARKET_BERU);
//         }
//     } else if (resPath == "personal_assistant.scenarios.skill_recommendation")
//     return resPath;
// }

TMaybe<TCard> ParseCard(const TSpeechKitResponseProto::TResponse::TCard& card) {
    if (!card.HasText() || !card.HasType()) {
        return {};
    }
    TCard resCard;
    resCard.Type = card.GetType();
    resCard.Text = card.GetText();
    if (resCard.Type == DIV_CARD) {
        for (const auto& action : NImpl::ParseElement(JsonFromProto(card.GetBody()), /* onlyLogId= */ false)) {
            resCard.CardId = action.CardId;
            resCard.IntentName = action.IntentName;
            if (resCard.IntentName) {
                ReplaceAll(*resCard.IntentName, ".", "\t");
                ReplaceAll(*resCard.IntentName, "__", "\t");
            }
            resCard.Actions.push_back(action.ActionName);
        }
    }
    return resCard;
}

TVector<TCard> ParseCards(const TVector<TSpeechKitResponseProto::TResponse::TCard>& cards) {
    TVector<TCard> resCards;
    for (const auto& card : cards) {
        auto resCard = ParseCard(card);
        if (resCard) {
            resCards.emplace_back(std::move(*resCard));
        }
    }
    return resCards;
}

} // namespace NAlice::NWonderSdk
