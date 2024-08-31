#include "music.h"
#include "fairy_tales.h"
#include "websearch.h"

#include <alice/bass/forms/setup_context.h>
#include <alice/bass/forms/music/providers.h>
#include <alice/bass/forms/music/cache.h>
#include <alice/bass/forms/urls_builder.h>
#include <alice/bass/forms/common/personal_data.h>

#include <alice/bass/libs/config/config.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/metrics/metrics.h>
#include <alice/bass/libs/scheduler/scheduler.h>

#include <alice/bass/setup/setup.h>

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/music/defs.h>

#include <library/cpp/langs/langs.h>
#include <library/cpp/neh/neh.h>

#include <library/cpp/cgiparam/cgiparam.h>

namespace NBASS::NMusic {

namespace {

constexpr TStringBuf YAMUSIC = "yamusic";
constexpr TStringBuf YARADIO = "yaradio";
constexpr TStringBuf SNIPPET = "snippet";
constexpr TStringBuf QUASAR = "quasar";

using TProviderFactory = std::function<IMusicProvider*(TContext&)>;
using TProvidersHash = THashMap<TStringBuf, TProviderFactory>;

EClientType GetClientType(const NAlice::TClientFeatures& clientFeatures) {
    if (clientFeatures.IsSmartSpeaker()) {
        return EClientType::SmartSpeaker;
    }

    if (clientFeatures.IsSearchApp()) {
        return EClientType::SearchApp;
    }

    if (clientFeatures.IsNavigator()) {
        return EClientType::Navi;
    }

    if (clientFeatures.IsYaAuto()) {
        return EClientType::Auto;
    }

    return EClientType::Other;
}

struct TOriginalIntentInfo {
    TString OriginalFormName;
    EClientType ClientType;

    NMonitoring::TLabels CreateLabels() const {
        NMonitoring::TLabels labels;
        labels.Add("music_play_original_form", OriginalFormName);
        labels.Add("client_type", ToString(ClientType));
        return labels;
    }
};

TOriginalIntentInfo CreateOriginalMusicIntentInfo(const TString& originalFormName, const NAlice::TClientFeatures& clientFeatures) {
    return {
        originalFormName,
        GetClientType(clientFeatures),
    };
}

TStringBuf SelectProviderNameByAnswerType(TStringBuf answerType, bool quasarLogic) {
    if (quasarLogic) {
        return QUASAR;
    }

    return (answerType == TStringBuf("radio") || answerType == TStringBuf("playlist"))
        ? YARADIO
        : YAMUSIC;
}

class TMusicContinuation : public THollywoodContinuation {
public:
    using THollywoodContinuation::THollywoodContinuation;

    TStringBuf GetName() const override {
        return TStringBuf("TMusicContinuation");
    }

    static TMaybe<TMusicContinuation> FromJson(NSc::TValue value, TGlobalContextPtr globalContext, NSc::TValue meta,
                                               const TString& authHeader, const TString& /* appInfoHeader */,
                                               const TString& /* fakeTimeHeader */,
                                               const TMaybe<TString>& userTicketHeader,
                                               const NSc::TValue& configPatch)
    {
        return TMusicContinuation(value, globalContext, meta, authHeader, userTicketHeader, configPatch);

    }

protected:
    TResultValue Apply() override;
};

class TMusicRequestImpl {
public:
    TMusicRequestImpl(TContext& ctx, bool skipCallback);
    TResultValue Do();
    TResultValue Prepare(TMusicContinuationPayload& payload);
    TResultValue Apply(TMusicContinuationPayload& payload);

private:
    TContext& Ctx;
    NSc::TValue SlotData;

    std::unique_ptr<IMusicProvider> Provider;

    bool QuasarLogic = false;
    bool SkipCallback = false;

private:
    TResultValue DoImpl(TMusicContinuationPayload& payload, NImpl::EProtocolStage stage);
    TResultValue DoImplInternal(TMusicContinuationPayload& payload, NImpl::EProtocolStage stage);
    TStringBuf DetectProvider();
    TResultValue InitProvider(TStringBuf providerName);
    TResultValue AnalyzeResult(NSc::TValue& output);
    void MakeCommandBlocks(NSc::TValue& output);
};

const TProvidersHash MUSIC_PROVIDERS = {
    {YAMUSIC, [](TContext& ctx) { return new TYandexMusicProvider(ctx); }},
    {YARADIO, [](TContext& ctx) { return new TYandexRadioProvider(ctx); }},
    {SNIPPET, [](TContext& ctx) { return new TSnippetProvider(ctx); }},
    {QUASAR, [](TContext& ctx) { return new TQuasarProvider(ctx); }},
};

constexpr TStringBuf AUTOMOTIVE_YANDEX_MUSIC_APP = "com.yandex.music.auto";

const THashMap<TStringBuf, TString> PRODUCT_SCENARIO_NAMES = {
    {"personal_assistant.scenarios.player_continue", NAlice::NProductScenarios::PLAYER_COMMANDS},
    {"personal_assistant.scenarios.music_sing_song", NAlice::NProductScenarios::PROMO},
    {"personal_assistant.scenarios.music_sing_song__next", NAlice::NProductScenarios::PROMO},
};

bool IsSupported(TContext& ctx, TStringBuf providerName) {
    if (ctx.ClientFeatures().SupportsMusicQuasarClient() || ctx.ClientFeatures().SupportsAudioClient()) {
        return true;
    } else if (ctx.MetaClientInfo().IsYaAuto()) {
        if (providerName == YARADIO) {
            return true;
        }
        return IsYaMusicSupported(ctx);
    } else if (ctx.MetaClientInfo().IsTvDevice() || ctx.MetaClientInfo().IsLegatus()) {
        return false;
    }
    return true;
};

TResultValue ProcessAnswerError(const TResultValue& error, TContext& ctx) {
    if (error->Type == TError::EType::MUSICERROR || error->Type == TError::EType::UNAUTHORIZED) {
        if (error->Msg != ERROR_DO_NOT_SEND) {
            ctx.AddErrorBlockWithCode(*error, error->Msg);
        }
        return TResultValue();
    } else {
        return error;
    }
}

// DIALOG-5329
bool IsYaAutoAuthorizationRequired(TContext& ctx) {
    return !ctx.IsAuthorizedUser() && ctx.MetaClientInfo().IsYaAuto() &&
        ctx.MetaClientInfo().IsAppOfVersionOrNewer(1, 6, 2);
}

bool EnsureAvailability(const TStringBuf provider, TContext& ctx) {
    if (IsYaAutoAuthorizationRequired(ctx)) {
        ctx.AddErrorBlock(TError(TError::EType::UNAUTHORIZED, ERROR_UNAUTHORIZED));
        return false;
    }

    if (provider == QUASAR && ctx.MetaClientInfo().IsNavigator()) {
        if (!ctx.IsAuthorizedUser()) {
            ctx.AddErrorBlockWithCode(TError(TError::EType::UNAUTHORIZED), TStringBuf("unauthorized_general"));
            return false;
        }

        TPersonalDataHelper::TUserInfo userInfo;
        if (!TPersonalDataHelper(ctx).GetUserInfo(userInfo) || !userInfo.GetHasYandexPlus()) {
            if (ctx.ParentFormName() == "personal_assistant.scenarios.music_fairy_tale" && ctx.HasExpFlag("fairy_tale_yaplus_response")) {
                ctx.AddAttention(TStringBuf("fairy_tale_noplus"));
            }
            ctx.AddErrorBlockWithCode(TError(TError::EType::UNAUTHORIZED), PAYMENT_REQUIRED);
            return false;
        }
    }

    return true;
}

// TODO(a-square): reduce duplication by passing the text query from the run stage and into the apply stage
TString CreateTextQuery(TContext& ctx, const NSc::TValue& slotData, const TString& utterance, const bool alarm) {
    if (DataGetHasKey(slotData, SNIPPET_SLOTS)) {
        const auto answer = TSnippetProvider::MakeDataFromSnippet(ctx, /* autoplay */ false, slotData[SNIPPET_SLOTS]["snippet"]);
        return TString{
            answer["title"].IsNull()
                ? answer["name"].GetString()
                : answer["title"].GetString()
        };
    }

    if (TString spName{slotData[CUSTOM_SLOTS]["special_playlist"].GetString()}) {
        if (spName == TStringBuf("alice") &&
            ctx.MetaClientInfo().IsSmartSpeaker() &&
            ctx.ClientFeatures().SupportsMusicPlayerAllowShots()
        ) {
            // XXX(a-square): duplicates some logic in TQuasarProvider
            spName = TStringBuf("origin");
        }

        if (!PERSONAL_SPECIAL_PLAYLISTS_DATA.contains(spName)) {
            TYandexMusicAnswer spAnswer(ctx.ClientFeatures());
            NSc::TValue answer;
            if (spAnswer.AnswerWithSpecialPlaylist(spName, false, &answer)) {
                return TString{answer["title"].GetString()};
            }
        }
    }

    return MergeTextFromSlots(ctx, utterance, alarm);
}

[[nodiscard]] TResultValue RunSearch(TContext& ctx,
                                     TMusicContinuationPayload& payload,
                                     const NSc::TValue& slotData,
                                     const NSc::TValue& actionData,
                                     const bool alarm) {
    const auto textQuery = CreateTextQuery(ctx, slotData, TString{ctx.Meta().Utterance()}, alarm);

    LOG(INFO) << "SearchMusic: utterance = " << textQuery << Endl;

    TCheckAndSearchMusicParams params {
        .HasSearch = ShouldUseWebSearch(ctx.ClientFeatures(), slotData, actionData),
        .RichTracks = (
            ctx.ParentFormName() == "personal_assistant.scenarios.music_fairy_tale"sv ||
            ctx.HasExpFlag(EXPERIMENTAL_FLAG_MUSIC_RICH_TRACKS)
        ),
        .TextQuery = std::move(textQuery),
        .Ctx = ctx,
        .RequestStartTime = ctx.GetRequestStartTime(),
        .AnalyticsInfoBuilder = ctx.GetAnalyticsInfoBuilder(),
        .ClientFeatures = ctx.ClientFeatures(),
    };

    auto result = CheckAndSearchMusic(params);

    if (result.WebSearchMilliseconds) {
        ctx.AddStatsCounter("WebSearch_search_milliseconds", *result.WebSearchMilliseconds);
    }

    if (result.CatalogMilliseconds) {
        ctx.AddStatsCounter("WebSearch_catalog_bulk_milliseconds", *result.CatalogMilliseconds);
    }

    for (const auto& timing : result.CatalogTimings) {
        // XXX(a-square): because we can't duplicate stat names, only one entity of each type will make it,
        // should be at least enough to see if any one type is significantly slower than others
        const TString name = (TStringBuilder{} << "WebSearch_catalog_" << timing.Type << "_milliseconds");
        ctx.AddStatsCounter(name, timing.TimeMilliseconds);
    }

    payload.ApplyArguments[WEB_ANSWER] = std::move(result.WebAnswer);
    payload.ApplyArguments[MULTIROOM_ROOM] =  slotData[CUSTOM_SLOTS][NAlice::NMusic::SLOT_LOCATION];
    payload.ApplyArguments[MULTIROOM_LOCATION_ROOMS] = slotData[CUSTOM_SLOTS][NAlice::NMusic::SLOT_LOCATION_ROOM];
    payload.ApplyArguments[MULTIROOM_LOCATION_GROUPS] = slotData[CUSTOM_SLOTS][NAlice::NMusic::SLOT_LOCATION_GROUP];
    payload.ApplyArguments[MULTIROOM_LOCATION_DEVICES] = slotData[CUSTOM_SLOTS][NAlice::NMusic::SLOT_LOCATION_DEVICE];
    payload.ApplyArguments[MULTIROOM_LOCATION_EVERYWHERE] = slotData[CUSTOM_SLOTS][NAlice::NMusic::SLOT_LOCATION_EVERYWHERE];
    payload.ApplyArguments[MULTIROOM_LOCATION_SMART_SPEAKER_MODELS] = slotData[CUSTOM_SLOTS][NAlice::NMusic::SLOT_LOCATION_SMART_SPEAKER_MODEL];
    payload.ApplyArguments[ACTIVATE_MULTIROOM] = slotData[CUSTOM_SLOTS][NAlice::NMusic::SLOT_ACTIVATE_MULTIROOM];
    payload.FeaturesData = std::move(result.FeaturesData);
    payload.NeedApply = !result.Result;

    // music_not_found is not really an error
    auto& error = result.Result;
    if (error && error->Type == TError::EType::MUSICERROR && error->Msg == ERROR_MUSIC_NOT_FOUND) {
        if (IsFairyTaleFilterGenre(ctx)) {
            ctx.AddAttention("unknown_fairy_tale");
        }
        ctx.AddErrorBlockWithCode(*error, ERROR_MUSIC_NOT_FOUND);
        return TResultValue();
    }

    return std::move(error);
}

TResultValue GetAnswer(IMusicProvider& provider,
                       TContext& ctx,
                       NSc::TValue* out,
                       TMusicContinuationPayload& payload,
                       const NSc::TValue& slotData,
                       const NSc::TValue& actionData,
                       const bool alarm,
                       const NImpl::EProtocolStage stage) {
    switch (stage) {
        case NImpl::EProtocolStage::Legacy: {
            if (const auto resultValue = RunSearch(ctx, payload, slotData, actionData, alarm)) {
                return resultValue;
            }

            if (payload.NeedApply) {
                return provider.Apply(out, std::move(payload.ApplyArguments));
            }
            return TResultValue();
        }
        case NImpl::EProtocolStage::Run: {
            return RunSearch(ctx, payload, slotData, actionData, alarm);
        }
        case NImpl::EProtocolStage::Apply: {
            Y_ENSURE(payload.NeedApply);
            return provider.Apply(out, std::move(payload.ApplyArguments));
        }
    }
}

} // end anon namespace

TResultValue GetAnswer(IMusicProvider& provider,
                       TContext& ctx,
                       NSc::TValue* out,
                       const NSc::TValue& slotData,
                       const NSc::TValue& actionData,
                       const bool alarm) {
    TMusicContinuationPayload payload; // not really used
    return GetAnswer(provider, ctx, out, payload, slotData, actionData, alarm, NImpl::EProtocolStage::Legacy);
}

bool IsYaMusicSupported(TContext& ctx) {
    // https://st.yandex-team.ru/ALICE-4456#5de674ccb435632b90b6d05f
    const i64 musicSupportAppBuildNumber = 23019560;
    const i64 oldAutoClientsAppBuildNumber = 19018404;
    if (ctx.Meta().DeviceState().HasInstalledApps()) {
        const auto apps = ctx.Meta().DeviceState().InstalledApps();
        if (apps.IsDict()) {
            for (const auto& kv : apps->GetDict()) {
                if (kv.first == AUTOMOTIVE_YANDEX_MUSIC_APP) {
                    auto buildNumber = kv.second.ForceIntNumber(0);
                    return buildNumber <= oldAutoClientsAppBuildNumber || buildNumber >= musicSupportAppBuildNumber;
                }
            }
        }
    }
    return false;
}

bool FillParams(TContext& ctx, const TVector<TStringBuf>& slotNames, bool needArray, NSc::TValue* params) {
    bool hasAnyData = false;
    for (const auto& sName : slotNames) {
        const TContext::TSlot* slot = ctx.GetSlot(sName);
        if (IsSlotEmpty(slot)) {
            continue;
        }

        const NSc::TValue& value = slot->Value;
        if (value.IsString() && value.GetString().empty()) {
            continue;
        }

        hasAnyData = true;
        if (needArray) {
            (*params)[sName].Push() = value;
        } else {
            (*params)[sName] = value;
        }
    }
    return hasAnyData;
}


void FillNewLocationSlotsFromTheOldOne(TContext& ctx, NSc::TValue& slotData) {
    auto& customSlotsData = slotData[CUSTOM_SLOTS];

    const auto* slot = ctx.GetSlot(NAlice::NMusic::SLOT_LOCATION);

    if (IsSlotEmpty(slot) || slot->Value.GetString().empty()) {
        return;
    }

    if (slot->Type == NAlice::NMusic::SLOT_LOCATION_ROOM_TYPE && slot->Value.GetString() != "everywhere") {
        customSlotsData[NAlice::NMusic::SLOT_LOCATION_ROOM].Push(slot->Value);
    } else if (slot->Type == NAlice::NMusic::SLOT_LOCATION_GROUP_TYPE) {
        customSlotsData[NAlice::NMusic::SLOT_LOCATION_GROUP].Push(slot->Value);
    } else if (slot->Type == NAlice::NMusic::SLOT_LOCATION_DEVICE_TYPE) {
        customSlotsData[NAlice::NMusic::SLOT_LOCATION_DEVICE].Push(slot->Value);
    } else if (slot->Type == NAlice::NMusic::SLOT_LOCATION_SMART_SPEAKER_MODEL_TYPE) {
        customSlotsData[NAlice::NMusic::SLOT_LOCATION_SMART_SPEAKER_MODEL].Push(slot->Value);
    } else if (slot->Type == NAlice::NMusic::SLOT_LOCATION_EVERYWHERE_TYPE || slot->Value.GetString() == "everywhere") {
        customSlotsData[NAlice::NMusic::SLOT_LOCATION_EVERYWHERE].Push(true);
    }
}


void FillSlotData(TContext& ctx, NSc::TValue& slotData) {
    for (const auto& slotKind : { SEARCH_SLOTS, OBJECT_SLOTS, MAIN_FILTERS_SLOTS, OTHER_FILTERS_SLOTS, SNIPPET_SLOTS, RESULT_SLOTS, RADIO_SEEDS_SLOTS, CUSTOM_SLOTS }) {
        NSc::TValue& section = slotData[slotKind];
        bool hasAnySlot = FillParams(
            ctx,
            SLOT_NAMES.find(slotKind)->second,
            (slotKind == MAIN_FILTERS_SLOTS || slotKind == OTHER_FILTERS_SLOTS),
            &section
        );
        DataSetHasKey(slotData, slotKind, hasAnySlot);
    }
    FillNewLocationSlotsFromTheOldOne(ctx, slotData);
}

TMusicRequestImpl::TMusicRequestImpl(TContext& ctx, const bool skipCallback)
    : Ctx(ctx)
    , QuasarLogic(ctx.ClientFeatures().SupportsMusicPlayer())
    , SkipCallback(skipCallback)
{
}

TStringBuf TMusicRequestImpl::DetectProvider() {
    if (QuasarLogic) {
        return QUASAR;
    }

    const auto& playlistId = SlotData.TrySelect("object/playlist_id");
    if (playlistId.IsString()) {
        SlotData[CUSTOM_SLOTS]["special_playlist"] = playlistId;
        DataSetHasKey(SlotData, CUSTOM_SLOTS, true);
    } else if (DataGetHasKey(SlotData, OBJECT_SLOTS)) {
        return TStringBuf(""); // We do not support object_ids in non-Quasar providers
    }

    if (DataGetHasKey(SlotData, RESULT_SLOTS) ||
        DataGetHasKey(SlotData, SEARCH_SLOTS) ||
        DataGetHasKey(SlotData, OTHER_FILTERS_SLOTS) ||
        (DataGetHasKey(SlotData, CUSTOM_SLOTS) && (
            SlotData[CUSTOM_SLOTS].Has("special_playlist") ||
            SlotData[CUSTOM_SLOTS].Has(NAlice::NMusic::SLOT_SPECIAL_ANSWER_INFO) ||
            SlotData[CUSTOM_SLOTS].Has("novelty") && SlotData["custom"]["novelty"].GetString() == TStringBuf("new")
        ))) {
        return YAMUSIC;
    }

    if (DataGetHasKey(SlotData, SNIPPET_SLOTS)) {
        return SNIPPET;
    }

    if (!DataGetHasKey(SlotData, MAIN_FILTERS_SLOTS) || CheckRadioFilters(SlotData[MAIN_FILTERS_SLOTS])) {
        return YARADIO;
    }

    return YAMUSIC;
}


TResultValue TMusicRequestImpl::AnalyzeResult(NSc::TValue& output) {
    if (output.IsNull()) {
        return TError(
            TError::EType::MUSICERROR,
            TStringBuf("empty_service_answer")
        );
    }
    if (!QuasarLogic && output.Get("uri").GetString().empty()) {
        return TError(
            TError::EType::MUSICERROR,
            TStringBuf("no_uri_in_answer")
        );
    }
    return TResultValue();
}

TResultValue TMusicRequestImpl::InitProvider(TStringBuf providerName) {
    if (!MUSIC_PROVIDERS.contains(providerName)) {
        LOG(ERR) << "Can't find provider with name " << providerName << Endl;
        return TError(
            TError::EType::SYSTEM,
            TStringBuf("cant_find_provider_for_input")
        );
    }

    if (providerName == QUASAR && Ctx.MetaClientInfo().IsSmartSpeaker() && Ctx.UserAuthorizationHeader().Empty()) {
        return TError(
            TError::EType::UNAUTHORIZED,
            TStringBuf("cant_support_music_on_smart_speakers_without_authorization")
        );
    }

    Provider.reset(MUSIC_PROVIDERS.find(providerName)->second(Ctx));
    if (!Provider) {
        return TError(
            TError::EType::SYSTEM,
            TStringBuf("cannot_create_provider")
        );
    }

    if (!Provider->InitRequestParams(SlotData)) {
        return TError(
            TError::EType::MUSICERROR,
            TStringBuf("cannot_init_provider")
        );
    }

    return TResultValue();
}

void TMusicRequestImpl::MakeCommandBlocks(NSc::TValue& output) {
    TStringBuf answerType = output.Get("type").GetString();

    if (Provider) {
        Provider->MakeBlocks(output);

        // Hack for other suggests for playlists
        if (!QuasarLogic && answerType == TStringBuf("playlist")) {
            if (TResultValue err = InitProvider(YARADIO)) {
                LOG(ERR) << "MUSIC error: " << err->Msg << Endl;
                return;
            }
        }
    } else {
        if (TResultValue err = InitProvider(SelectProviderNameByAnswerType(answerType, QuasarLogic))) {
            LOG(ERR) << "MUSIC error: " << err->Msg << Endl;
            return;
        }

        Provider->MakeBlocks(output);
    }

    Provider->AddSuggest(output);
}

TResultValue TMusicRequestImpl::DoImplInternal(TMusicContinuationPayload& payload,
                                               const NMusic::NImpl::EProtocolStage stage) {
    if (stage == NImpl::EProtocolStage::Legacy) {
        LOG(INFO) << "Got request from Vins music" << Endl;
        Ctx.SetDontResubmit();  // makes sure that forms like player_continue don't end up calling music_play twice
        Ctx.SetIsMusicVinsRequest();
    }

    const auto originalIntentInfo = CreateOriginalMusicIntentInfo(Ctx.OriginalFormName(), Ctx.ClientFeatures());
    LOG(INFO) << "music_play_original_form: OriginalFormName = " << originalIntentInfo.OriginalFormName
              << ", ClientType = " << originalIntentInfo.ClientType << Endl;
    Ctx.GlobalCtx().Counters().Sensors().Rate(originalIntentInfo.CreateLabels())->Inc();

    // XXX(a-square): currently, the answer slot is not supported by Hollywood,
    // so in the HollywoodMusic's logic, we never go into this branch, so it's okay to put it
    // before the music search
    // TODO(a-square): figure out how to deal with the answer slot (used for suggests and maybe something else?)
    TContext::TSlot* slotResult = Ctx.GetSlot(TStringBuf("answer"), TStringBuf("music_result"));
    if (!IsSlotEmpty(slotResult)) {
        if (QuasarLogic) {
            Ctx.CreateSlot("track_id", "string", true, slotResult->Value.TrySelect("id"));
            Ctx.DeleteSlot("answer");
        } else {
            DataSetHasKey(SlotData, RESULT_SLOTS, true);
            MakeCommandBlocks(slotResult->Value);
            return TResultValue();
        }
    }

    // XXX(a-square): having SlotData in the first place is bad, but as long as we have it,
    // it must be filled before either RunSearch or DetectProvider is called.
    // TODO(a-square): Ideally, we should refactor everything so that slots are **immutable**
    // and never have to be changed once while processing the form.
    FillSlotData(Ctx, SlotData);

    switch (stage) {
        case NImpl::EProtocolStage::Legacy: {
            if (const auto resultValue = RunSearch(Ctx, payload, SlotData, /* actionData= */ {}, /* alarm= */ false)) {
                return resultValue;
            }
            // proceed to apply
            break;
        }
        case NImpl::EProtocolStage::Run: {
            // early exit
            return RunSearch(Ctx, payload, SlotData, /* actionData= */ {}, /* alarm= */ false);
            break;
        }
        case NImpl::EProtocolStage::Apply: {
            // proceed to apply
            break;
        }
    }

    TStringBuf providerName = DetectProvider();
    LOG(INFO) << "Detected music provider: " << providerName << Endl;

    // XXX(a-square): I switched up the order of checks so a case of music not found
    // will be reported to the user before we have a chance of seeing if the provider is supported,
    // so in some rare cases there might be user confusion because of it.
    // I don't think it's possible to avoid this without code duplication, I'm not sure it's worthwhile
    if (!IsSupported(Ctx, providerName)) {
        Ctx.AddStopListeningBlock();
        Ctx.AddAttention(TStringBuf("music_play_not_supported_on_device"), {});
        return TResultValue();
    }

    if (!EnsureAvailability(providerName, Ctx)) {
        return TResultValue();
    }

    if (TResultValue errInit = InitProvider(providerName)) {
        if (errInit->Type == TError::EType::SYSTEM || errInit->Type == TError::EType::UNAUTHORIZED) {
            return errInit;
        }
        Ctx.AddErrorBlockWithCode(*errInit, TStringBuf("params_problem"));
        return TResultValue();
    }

    NSc::TValue outputValue;

    if (payload.NeedApply) {
        if (TResultValue errRequest = Provider->Apply(&outputValue, std::move(payload.ApplyArguments))) {
            return ProcessAnswerError(errRequest, Ctx);
        }
    }

    if (TResultValue errContent = AnalyzeResult(outputValue)) {
        Ctx.AddErrorBlockWithCode(*errContent, ERROR_MUSIC_NOT_FOUND);
        return TResultValue();
    }

    MakeCommandBlocks(outputValue);
    Ctx.CreateSlot("answer", "music_result", true, std::move(outputValue));

    // NOTE(a-square): VINS interface doesn't have a dedicated features field so we add a block instead.
    // We chose not to create a more general mechanism for the legacy VINS interface because currently
    // only music and video scenarios need feature data, and there is no indication that more scenarios
    // will need their own features.
    if (stage == NMusic::NImpl::EProtocolStage::Legacy && !payload.FeaturesData.IsNull()) {
        auto& block = *Ctx.Block();
        block["type"] = "features_data";
        block["data"] = std::move(payload.FeaturesData);
    }

    return TResultValue();
}

TResultValue TMusicRequestImpl::DoImpl(TMusicContinuationPayload& payload, const NMusic::NImpl::EProtocolStage stage) {
    auto res = DoImplInternal(payload, stage);

    if (!Ctx.MetaClientInfo().IsSmartSpeaker() && !Ctx.MetaClientInfo().IsTvDevice() && stage != NMusic::NImpl::EProtocolStage::Run) {
        Ctx.AddSearchSuggest();
        Ctx.AddOnboardingSuggest();
    }

    if (!SkipCallback) {
        if (Ctx.SetCallbackAsResponseFormAndCopySlots()) {
            return Ctx.RunResponseFormHandler();
        }
    }

    return res;
}

TResultValue TMusicRequestImpl::Do() {
    TMusicContinuationPayload payload; // used to pass features around
    return DoImpl(payload, NMusic::NImpl::EProtocolStage::Legacy);
}

TResultValue TMusicRequestImpl::Prepare(TMusicContinuationPayload& payload) {
    return DoImpl(payload, NMusic::NImpl::EProtocolStage::Run);
}

TResultValue TMusicRequestImpl::Apply(TMusicContinuationPayload& payload) {
    return DoImpl(payload, NMusic::NImpl::EProtocolStage::Apply);
}

TResultValue TSearchMusicHandler::Do(TRequestHandler& r) {
    const auto* productScenarioName = PRODUCT_SCENARIO_NAMES.FindPtr(r.Ctx().OriginalFormName());
    if (!productScenarioName) {
        productScenarioName = &NAlice::NProductScenarios::MUSIC;
    }
    r.Ctx().GetAnalyticsInfoBuilder().SetProductScenarioName(*productScenarioName);

    TMusicRequestImpl impl(r.Ctx(), /* skipCallback */ false); // TRequestHandler owns and fills the context
    return impl.Do();
}

IContinuation::TPtr TSearchMusicHandler::Prepare(TRequestHandler& r) {
    // TODO(a-square):
    // - Because we're using the post-classifier, this thing may in fact be misclassifying some queries,
    //   notably podcasts and fairy tales. Consider separating those during the apply stage (may be hairy).
    // - Eventually, the prepare stage will be replaced in favor of AppHost-only run stage,
    //   make sure to port this logic there!
    r.Ctx().GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::MUSIC);

    TMusicRequestImpl impl(r.Ctx(), /* skipCallback */ false); // TRequestHandler owns and fills the context

    TMusicContinuationPayload payload;
    const auto res = impl.Prepare(payload);

    if (!payload.NeedApply) {
        return TCompletedContinuation::Make(r.Ctx(), res);
    }

    return std::make_unique<TMusicContinuation>(&r.Ctx(), std::move(payload.ApplyArguments),
                                                std::move(payload.FeaturesData));
}

// static
void TSearchMusicHandler::Register(THandlersMap* handlers) {
    handlers->RegisterFormAndContinuableHandler<TSearchMusicHandler>(MUSIC_PLAY_FORM_NAME);
    handlers->RegisterFormHandler(MUSIC_PLAY_MORE_FORM_NAME, []() { return MakeHolder<TSearchMusicHandler>(); });
    handlers->RegisterFormHandler(MUSIC_PLAY_LESS_FORM_NAME, []() { return MakeHolder<TSearchMusicHandler>(); });
}

// static
TResultValue TSearchMusicHandler::DoWithoutCallback(TContext& ctx) {
    TMusicRequestImpl impl(ctx, /* skipCallback */ true);
    return impl.Do();
}

// static
TContext::TPtr TSearchMusicHandler::SetAsResponse(TContext& ctx, bool callbackSlot, NSc::TValue snippet) {
    TContext::TPtr newCtx = ctx.SetResponseForm(MUSIC_PLAY_FORM_NAME, callbackSlot);
    Y_ENSURE(newCtx);

    if (snippet.IsNull()) {
        return newCtx;
    }

    newCtx->CreateSlot(TStringBuf("snippet"), TStringBuf("dict"), true, snippet);
    return newCtx;
}

// static
TContext::TPtr TSearchMusicHandler::SetAsResponse(TContext& ctx, bool callbackSlot) {
    TContext::TPtr newCtx = ctx.SetResponseForm(MUSIC_PLAY_FORM_NAME, callbackSlot);
    Y_ENSURE(newCtx);

    return newCtx;
}

NSc::TValue TSearchMusicHandler::CreateMusicDataFromSnippet(TContext& ctx, const NSc::TValue& snippet) {
    // Autoplay = true for suggest in Searchapp
    NSc::TValue data = TSnippetProvider::MakeDataFromSnippet(ctx, /* autoplay */ true, snippet);
    if (!data.IsNull()) {
        NSc::TValue musicplayer;
        musicplayer["url"] = data["uri"];
        return musicplayer;
    }
    return NSc::Null();
}

TResultValue TMusicContinuation::Apply() {
    TMusicRequestImpl impl(GetContext(), /* skipCallback */ false); // the continuation owns the context

    TMusicContinuationPayload payload;
    payload.NeedApply = !IsFinished();
    payload.ApplyArguments = std::move(ApplyArguments_);
    payload.FeaturesData = std::move(FeaturesData_);
    return impl.Apply(payload);
}

void TMusicPlayObjectActionHandler::Register(THandlersMap* handlers) {
    handlers->RegisterActionHandler(QUASAR_MUSIC_PLAY_OBJECT_ACTION_NAME, []() { return MakeHolder<TMusicPlayObjectActionHandler>(); });
}

TResultValue TMusicPlayObjectActionHandler::Do(TRequestHandler& r) {
    TContext& ctx = r.Ctx();
    auto& analyticsInfoBuilder = ctx.GetAnalyticsInfoBuilder();
    analyticsInfoBuilder.SetProductScenarioName(NAlice::NProductScenarios::MUSIC);
    analyticsInfoBuilder.SetIntentName(TString{QUASAR_MUSIC_PLAY_OBJECT_ACTION_STUB_INTENT});

    if (!ctx.MetaClientInfo().IsSmartSpeaker() && !ctx.ClientFeatures().SupportsMusicQuasarClient()) {
        ctx.AddErrorBlockWithCode(
            TError(TError::EType::NOTSUPPORTED, TStringBuf("action_is_not_supported")),
            TStringBuf("action_is_not_supported")
        );
        return TResultValue();
    }

    const NSc::TValue& actionData = ctx.InputAction().Get()->Data;
    const TStringBuf alarmId = actionData["alarm_id"].GetString();
    const bool filtered = actionData.Has("filters");
    if (alarmId) {
        ctx.AddSilentResponse();
    }

    TQuasarProvider provider(ctx);
    bool initializedProvider;

    NSc::TValue slotData;
    if (filtered) {
        auto& filters = actionData["filters"];
        if (!alarmId || !filters.IsDict() || filters.DictEmpty()) {
            return TError(TError::EType::SYSTEM, TStringBuf("unsupported_action_data_format"));
        }

        if (ctx.UserAuthorizationHeader().Empty()) {
            return TError(
                TError::EType::UNAUTHORIZED,
                TStringBuf("cant_support_music_on_smart_speakers_without_authorization")
            );
        }

        // Fill slots to fill slotData from them
        for (const auto& filter : filters.DictKeys()) {
            ctx.CreateSlot(filter, filter, /* optional */ true, filters[filter]);
        }

        FillSlotData(ctx, slotData);
        initializedProvider = provider.InitRequestParams(slotData);
    } else {
        initializedProvider = provider.InitWithActionData(actionData);
    }

    if (!initializedProvider) {
        auto error = TError(TError::EType::SYSTEM, TStringBuf("unsupported_action_data_format"));
        if (alarmId) {
            return error;
        }
        ctx.AddErrorBlockWithCode(
            std::move(error),
            TStringBuf("unsupported_action_data_format")
        );
        return TResultValue();
    }

    NSc::TValue outputValue;
    if (alarmId) {
        outputValue["alarm_id"].SetString(alarmId);
    }

    if (TResultValue errRequest = GetAnswer(provider, ctx, &outputValue,
                                            slotData, actionData, /* alarm = */ !alarmId.empty())) {
        if (alarmId) {
            return errRequest;
        }
        auto newCtx = ctx.SetResponseForm("personal_assistant.scenarios.music_play", false);
        Y_ENSURE(newCtx);
        return ProcessAnswerError(errRequest, *newCtx);
    }

    if (filtered && outputValue.IsNull()) {
        return TError(
            TError::EType::MUSICERROR,
            TStringBuf("empty_service_answer")
        );
    }

    provider.MakeBlocks(outputValue);

    return TResultValue();
}

void RegisterMusicContinuations(TContinuationParserRegistry& registry) {
    registry.Register<TMusicContinuation>("TMusicContinuation");
}

} // namespace NBASS::NMusic
