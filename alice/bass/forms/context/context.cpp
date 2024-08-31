#include "context.h"

#include <alice/bass/forms/directives.h>
#include <alice/bass/forms/geo_resolver.h>
#include <alice/bass/forms/geocoder.h>
#include <alice/bass/forms/geodb.h>
#include <alice/bass/forms/request.h>
#include <alice/bass/forms/urls_builder.h>
#include <alice/bass/forms/vins.h>
#include <alice/bass/forms/common/personal_data.h>
#include <alice/bass/forms/video/utils.h>

#include <alice/bass/libs/analytics/analytics.h>
#include <alice/bass/libs/avatars/avatars.h>
#include <alice/bass/libs/eventlog/events.ev.pb.h>
#include <alice/bass/libs/fetcher/neh.h>
#include <alice/bass/libs/fetcher/serialization.h>
#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/metrics/metrics.h>
#include <alice/bass/libs/push_notification/request.h>
#include <alice/bass/libs/push_notification/create_callback_data.h>
#include <alice/bass/libs/push_notification/handlers/onboarding_push.h>
#include <alice/bass/libs/video_common/parsers/video_item.h>

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/json/json.h>
#include <alice/library/proto/protobuf.h>
#include <alice/library/util/rng.h>
#include <alice/library/video_common/defs.h>

#include <alice/megamind/protos/common/atm.pb.h>
#include <alice/megamind/protos/common/frame.pb.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/megamind/protos/scenarios/stack_engine.pb.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/neh/neh.h>
#include <library/cpp/timezone_conversion/convert.h>

#include <util/draft/datetime.h>
#include <util/string/ascii.h>
#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/string/split.h>

namespace NBASS {
namespace {

using TFormScheme = NBASSRequest::TForm<TSchemeTraits>;

constexpr TStringBuf BLOCK_TYPE_UNIPROXY_ACTION = "uniproxy-action";
constexpr TStringBuf COMMAND_TTS_PLAY_PLACEHOLDER_TYPE = "tts_play_placeholder";

constexpr TStringBuf LOCALE_LANG_RU = "ru";
constexpr TStringBuf LOCALE_LANG_TR = "tr";
constexpr TStringBuf LOCALE_COUNTRY_RU = "RU";
constexpr TStringBuf LOCALE_COUNTRY_TR = "TR";

TMaybe<TInstant> ConstructInstantFromTimestamp(TStringBuf timestamp) {
    ui64 value;
    if (TryFromString(timestamp, value)) {
        return TInstant::Seconds(value);
    }
    return Nothing();
}

// Gets user location from meta["laas_region"] if meta["location"] is null or inaccurate.
void MergeLocationData(NSc::TValue& meta, TInstant now) {
    if (meta.Get("laas_region").IsNull()) {
        return;
    }

    NSc::TValue& location = meta["location"];
    const NSc::TValue& laasRegion = meta["laas_region"];
    if (!location.IsNull() &&
        location.Get("accuracy").GetIntNumber(0) <= laasRegion.Get("location_accuracy").GetIntNumber(0)) {
        return;
    }

    location["lat"] = laasRegion["latitude"].GetNumber(0);
    location["lon"] = laasRegion["longitude"].GetNumber(0);
    location["accuracy"] = laasRegion["location_accuracy"].GetIntNumber(0);

    ui64 currentTime = now.Seconds();
    ui64 laasTimestamp = laasRegion["location_unixtime"].GetIntNumber();
    location["recency"] = (currentTime > laasTimestamp) ? TDuration::Seconds(currentTime - laasTimestamp).MilliSeconds() : 0;
}

std::unique_ptr<TConfig> ConstructPatchedConfig(IGlobalContext& globalCtx, const TContext::TInitializer& initData) {
    if (initData.ConfigPatch.IsNull()) {
        return {};
    }

    return globalCtx.Config().PatchedConfig(initData.ConfigPatch);
}

NAlice::EContentSettings ParseContentSettings(TStringBuf contentSettings) {
    if (contentSettings == TStringBuf("safe")) {
        return NAlice::EContentSettings::safe;
    }
    if (contentSettings == TStringBuf("children")) {
        return NAlice::EContentSettings::children;
    }
    if (contentSettings == TStringBuf("without")) {
        return NAlice::EContentSettings::without;
    }
    return NAlice::EContentSettings::medium;
}

std::unique_ptr<NAlice::IRng> CreateRng(const TStringBuf rngSeedStr) {
    if (rngSeedStr.Empty()) {
        return std::make_unique<NAlice::TRng>();
    }
    auto seed = MultiHash(rngSeedStr);
    return std::make_unique<NAlice::TRng>(seed);
}

void AddVideoCommandToAnalyticsInfo(TContext& context, TStringBuf commandType, const NSc::TValue& data) {
    auto addObject = [&context](const NAlice::NScenarios::TAnalyticsInfo::TObject& object) {
        context.GetAnalyticsInfoBuilder().AddObject(object);
    };

    if (commandType == NAlice::NVideoCommon::COMMAND_SHOW_DESCRIPTION) {
        addObject(NAlice::NMegamind::GetAnalyticsObjectForDescription(data["item"]));
    } else if (commandType == NAlice::NVideoCommon::COMMAND_SHOW_GALLERY) {
        addObject(NAlice::NMegamind::GetAnalyticsObjectForGallery(data["items"]));
    } else if (commandType == NAlice::NVideoCommon::COMMAND_SHOW_SEASON_GALLERY) {
        addObject(NAlice::NMegamind::GetAnalyticsObjectForSeasonGallery(data["tv_show_item"], data["items"],
                                                                        data["season"].GetIntNumber()));
    }
}

TContext::TLocale PrepareLocale(const TStringBuf clientLocale, const TStringBuf userLang) {
    TStringBuf lang, country;
    if (clientLocale.TrySplit('-', lang, country) ||
        clientLocale.TrySplit('_', lang, country))
    {
        // just try to split both formats
    }

    if (userLang) {
        if (lang != userLang) {
            lang = userLang;
            country.Clear();
        }
    } else {
        if (!IsIn({LOCALE_LANG_RU, LOCALE_LANG_TR}, lang)) {
            lang = (country == LOCALE_COUNTRY_TR ? LOCALE_LANG_TR : LOCALE_LANG_RU);
            country.Clear();
        }
    }

    if (country.empty()) {
        if (lang == LOCALE_LANG_RU) {
            country = LOCALE_COUNTRY_RU;
        } else if (lang == LOCALE_LANG_TR) {
            country = LOCALE_COUNTRY_TR;
        }
    }

    auto result = TContext::TLocale();
    result.Lang = TString(lang);
    if (country) {
        result.FullLocale = TStringBuilder() << result.Lang << '-' << country;
    } else {
        result.FullLocale = result.Lang;
    }
    return result;
}

} // namespace

TContext::TBlock* TContext::Block() {
    return BlockList.emplace_back(std::make_unique<TBlock>()).get();
}

void TContext::AddBlock(NSc::TValue value) {
    BlockList.emplace_back(std::make_unique<TBlock>(value));
}

bool TContext::HasAnyBlockOfType(TStringBuf type) const {
    for (const auto& block : BlockList) {
        if (block->Get("type").GetString() == type) {
            return true;
        }
    }
    return false;
}

// TDiv2BlockBuilder ----------------------------------------------------------
TDiv2BlockBuilder::TDiv2BlockBuilder(NSc::TValue preRenderedCard, bool hideBorders)
    : Data_{std::move(preRenderedCard)}
    , HideBorders_{hideBorders}
{
}

TDiv2BlockBuilder::TDiv2BlockBuilder(const TString& cardTemplateName, NSc::TValue data, bool hideBorders)
    : CardName_{cardTemplateName}
    , Data_{std::move(data)}
    , HideBorders_{hideBorders}
{
}

TDiv2BlockBuilder& TDiv2BlockBuilder::UseTemplate(const TString& templateName) {
    TemplateNames_.push_back(templateName);
    return *this;
}
TDiv2BlockBuilder& TDiv2BlockBuilder::UseTemplate(const TString& templateName, NSc::TValue preRenderedTemplate) {
    PreRenderedTemplates_[templateName].Swap(preRenderedTemplate);
    return *this;
}
TDiv2BlockBuilder& TDiv2BlockBuilder::SetHideBorders(bool hideBorders) {
    HideBorders_ = hideBorders;
    return *this;
}

TDiv2BlockBuilder& TDiv2BlockBuilder::SetText(const TString& text) {
    Text_ = text;
    TextSource_ = ETextSource::Bass;
    return *this;
}

TDiv2BlockBuilder& TDiv2BlockBuilder::SetTextFromNlgPhrase(const TString& type) {
    Text_ = type;
    TextSource_ = ETextSource::VinsNlg;
    return *this;
}

void TDiv2BlockBuilder::ToBlock(NSc::TValue& block) && {
    block["type"].SetString("div2_card");
    if (CardName_.Defined()) {
        block["card_template"].SetString(*CardName_);
        block["data"].Swap(Data_);
    } else {
        block["body"].Swap(Data_);
    }
    block["hide_borders"].SetBool(HideBorders_);

    if (!Text_.empty()) {
        switch (TextSource_) {
            case ETextSource::Bass:
                block["text"].SetString(Text_);
                break;

            case ETextSource::VinsNlg:
                block["text_template"].SetString(Text_);
                break;

            case ETextSource::None:
                break;
        }
    }

    for (const auto& name : TemplateNames_) {
        block["template_names"].Push(name);
    }

    block["templates"].Swap(PreRenderedTemplates_);
}

// TContext -------------------------------------------------------------------
TContext::TContext(TStringBuf formName, TMaybe<TInputAction>&& inputAction, TSlotList&& slots,
                   const NSc::TValue& meta, TInitializer initData, const NSc::TValue& sessionState,
                   TSetupResponses&& setupResponses, TMaybe<NSc::TValue> blocks, TDataSources&& dataSources,
                   TStringBuf originalFormName, const TStringBuf parentFormName)
    : GlobalContext(initData.GlobalCtx)
    , FName(formName)
    , ParentFName(parentFormName)
    , OriginalFName(originalFormName)
    , InputActionParam(std::move(inputAction))
    , Config(ConstructPatchedConfig(GlobalCtx(), initData))
    , SlotList(std::move(slots))
    , ClientFeaturesBlock(nullptr)
    , StatsBlock(nullptr)
    , RequestMeta(meta)
    , RequestMetaScheme(&RequestMeta)
    , Features(RequestMetaScheme, *RequestMetaScheme.Experiments().Get())
    , ClientInfo(RequestMetaScheme)
    , Locale(PrepareLocale(ClientInfo.Lang, RequestMetaScheme.UserLang().Get()))
    , ReqDataSources(std::move(dataSources))
    , AuthorizationHeader(std::move(initData.AuthHeader))
    , AppInfoHeader(std::move(initData.AppInfoHeader))
    , UserIPAddress(RequestMetaScheme.ClientIP())
    , RequestId(std::move(initData.ReqId))
    , MarketRequestId(std::move(initData.MarketReqId))
    , UserTicketHeader(std::move(initData.UserTicketHeader))
    , FId(std::move(initData.Id))
    , RequestSessionState(sessionState)
    , RequestSessionStateScheme(TSessionState(&RequestSessionState))
    , SetupResponses(std::move(setupResponses))
    , RngSeed_(RequestMetaScheme.RngSeed().Get())
    , Rng(CreateRng(RngSeed_))
    , SpeechKitEvent(std::move(initData.SpeechKitEvent))
    , FakeTime(ConstructInstantFromTimestamp(initData.FakeTimeHeader))
    , IsClassifiedAsChildRequest_(IsClassifiedAsChildRequest())
    , ContentRestrictionLevel_(CalculateContentRestrictionLevel())
{
    for (auto& slot : SlotList) {
        SlotMap.emplace(slot->Name, slot.get());
    }

    for (const auto& permission : Meta().Permissions()) {
        if (permission.HasName() && permission.HasStatus()) {
            Permissions[permission.Name()] = permission.Status();
        }
    }

    if (blocks.Defined() && blocks->IsArray()) {
        auto& blocksArray = blocks->GetArray();
        for (size_t i = 0; i < blocksArray.size(); ++i)
            AddBlock(std::move(blocksArray[i]));
    }

    RequestStartTime =
        Meta().HasRequestStartTime() ? TInstant::MicroSeconds(Meta().RequestStartTime()) : Now();

    const auto forbiddenIntents = ExpFlag(NAlice::NExperiments::EXP_FORBIDDEN_INTENTS);
    if (forbiddenIntents.Defined()) {
        StringSplitter(*forbiddenIntents).Split(',').ParseInto(&ForbiddenIntents);
    }
}

TContext::TContext(const TContext& parentContext, const TStringBuf formName)
    : TContext(formName, /*inputAction*/ Nothing(), TSlotList(), parentContext.RequestMeta,
               TInitializer(parentContext), parentContext.RequestSessionState,
               TSetupResponses(), Nothing() /* blocks */, {} /* dataSources */,
               parentContext.OriginalFName, parentContext.FName)
{
    ChangeFormCounter += parentContext.ChangeFormCounter + 1;

    UserLocation_ = parentContext.UserLocation_;
    UserAddresses = parentContext.UserAddresses;

    if (parentContext.AnalyticsInfoBuilder.Defined()) {
        AnalyticsInfoBuilder = parentContext.AnalyticsInfoBuilder;
        // We should support generic behavior of a new AnalyticsInfoBuilder
        AnalyticsInfoBuilder->SetIntentName(FormName());
    }
}

TContext::~TContext() = default;

TContext::TPtr TContext::Clone() const {
    TSlotList slots;
    for (const auto& slot : SlotList) {
        slots.emplace_back(std::make_unique<TSlot>(*slot));
    }

    return new TContext(
        FormName(),
        TMaybe<TInputAction>(InputActionParam),
        std::move(slots),
        RequestMeta,
        TInitializer(*this),
        RequestSessionState,
        TSetupResponses(SetupResponses),
        Nothing() /* blocks */,
        TDataSources{ReqDataSources},
        OriginalFormName(),
        ParentFormName()
    );
}

// static
TResultValue TContext::FromJson(const NSc::TValue& request, TInitializer& initData, TContext::TPtr* context) {
    TConstructor constructor = [context](TStringBuf formName, TMaybe<TInputAction>&& inputAction,
                                         TSlotList&& slots,const NSc::TValue& meta, TInitializer initData,
                                         const NSc::TValue& sessionState, TSetupResponses&& setupResponses,
                                         TMaybe<NSc::TValue> blocks, TDataSources&& dataSources)
    {
        *context = new TContext{formName, std::move(inputAction), std::move(slots), meta, std::move(initData),
                                sessionState, std::move(setupResponses), std::move(blocks), std::move(dataSources),
                                /* originalFormName= */ formName};
        return Nothing();
    };

    return FromJsonImpl(request, std::move(initData), constructor, true /* shouldValidate */);
}

// static
TResultValue TContext::FromJsonUnsafe(const NSc::TValue& request, TInitializer& initData, TContext::TPtr* context) {
    TConstructor constructor = [context](TStringBuf formName, TMaybe<TInputAction>&& inputAction,
                                         TSlotList&& slots,const NSc::TValue& meta, TInitializer initData,
                                         const NSc::TValue& sessionState, TSetupResponses&& setupResponses,
                                         TMaybe<NSc::TValue> blocks, TDataSources&& dataSources)
    {
        *context = new TContext{formName, std::move(inputAction), std::move(slots), meta, std::move(initData),
                                sessionState, std::move(setupResponses), std::move(blocks), std::move(dataSources),
                                /* originalFormName= */ formName};
        return Nothing();
    };

    return FromJsonImpl(request, std::move(initData), constructor, false /* shouldValidate */);
}

const TConfig& TContext::GetConfig() const {
    if (Config) {
        return *Config;
    }
    return GlobalCtx().Config();
}

TSourcesRequestFactory TContext::GetSources() const {
    return TSourcesRequestFactory(GlobalCtx().Sources(), GetConfig(), Features.Experiments());
}

TContext::TBlock* TContext::AddSuggest(TStringBuf type, NSc::TValue data, NSc::TValue formUpdate) {
    TBlock* block = Block();
    (*block)["type"].SetString("suggest");
    (*block)["suggest_type"].SetString(type);
    if (!data.IsNull())
        (*block)["data"].Swap(data);
    if (!formUpdate.IsNull())
        (*block)["form_update"].Swap(formUpdate);
    return block;
}

NSc::TValue TContext::DeleteSuggest(TStringBuf type) {
    return DeleteBlockImpl(TStringBuf("suggest"), type);
}

void TContext::AddSearchSuggest(TStringBuf query) {
    // do not add search suggests to Navigator, Ya.Auto, Quasar (Ya.Statition)
    if (MetaClientInfo().IsNavigator() || MetaClientInfo().IsYaAuto() ||
        MetaClientInfo().IsSmartSpeaker() || MetaClientInfo().IsLegatus())
    {
        return;
    }

    if (TStringBuf utterance = (query ? query : Meta().Utterance())) {
        TSlot querySlot("query", "string");
        querySlot.Value = utterance;

        TSlot disableChangeIntentSlot("disable_change_intent", "bool");
        disableChangeIntentSlot.Value.SetBool(true);

        NSc::TValue formUpdate;
        formUpdate["name"] = "personal_assistant.scenarios.search";
        formUpdate["slots"].SetArray().Push(querySlot.ToJson(nullptr));
        formUpdate["slots"].Push(disableChangeIntentSlot.ToJson(nullptr));
        formUpdate["resubmit"].SetBool(true);

        NSc::TValue suggestData = NSc::Null();
        if (HasExpFlag(EXPERIMENTAL_FLAG_SEARCH_INTERNET_FALLBACK_SUGGEST_IMAGE)) {
            const TAvatar* avatar = Avatar(TStringBuf("search_suggest"), TStringBuf("ya_logo"));
            if (avatar) {
                suggestData["theme"]["image_url"] = avatar->Https;
            }
        }

        AddSuggest(TStringBuf("search_internet_fallback"), suggestData, formUpdate);
    }
}

void TContext::AddOnboardingSuggest() {
    if (MetaClientInfo().IsYaAuto()) {
        return;
    }
    AddSuggest("onboarding__what_can_you_do");
}

void TContext::AddFrameActionBlock(const TString& actionId, const NAlice::NScenarios::TFrameAction& frameAction) {
    const TString jsonFrameAction = NAlice::JsonStringFromProto(frameAction);

    NSc::TValue data;
    data["action_id"].SetString(actionId);
    data["frame_action"] = NSc::TValue::FromJson(jsonFrameAction);

    TBlock* block = Block();
    (*block)["type"].SetString("frame_action");
    (*block)["data"].Swap(data);
}

void TContext::AddScenarioDataBlock(const NAlice::NData::TScenarioData& scenarioData) {
    const TString jsonScenarioData = NAlice::JsonStringFromProto(scenarioData);
    auto data = NSc::TValue::FromJson(jsonScenarioData);

    TBlock* block = Block();
    (*block)["type"].SetString("scenario_data");
    (*block)["data"].Swap(data);
}

TContext::TBlock* TContext::AddAttention(TStringBuf type, NSc::TValue value) {
    TBlock* block = Block();
    (*block)["type"].SetString("attention");
    (*block)["attention_type"].SetString(type);
    (*block)["data"].Swap(value);
    return block;
}

TContext::TBlock* TContext::AddCountedAttention(TStringBuf type, NSc::TValue value) {
    Y_STATS_INC_COUNTER(type);
    return AddAttention(type, value);
}

NSc::TValue TContext::DeleteAttention(TStringBuf type) {
    return DeleteBlockImpl(TStringBuf("attention"), type);
}

bool TContext::HasAttention(TStringBuf type) const {
    return HasAnyAttention({ type });
}

bool TContext::HasAnyAttention(const TSet<TStringBuf>& types) const {
    return FindIf(BlockList, [&](const auto& block) {
        return block->Get("type").GetString() == TStringBuf("attention")
            && types.contains(block->Get("attention_type").GetString());
    }) != BlockList.end();
}

TContext::TBlock* TContext::AddServerAction(TStringBuf type, NSc::TValue data) {
    return AddCommandImpl("server_action" /* type */, type /* serverAction */, data /* data */);
}

TContext::TBlock* TContext::AddMementoUpdateBlock(NAlice::NScenarios::TMementoChangeUserObjectsDirective&& directive) {
    TBlock* block = Block();
    (*block)["type"].SetString(BLOCK_TYPE_UNIPROXY_ACTION);
    (*block)["command_type"].SetString("memento_change_user_objects_directive");
    (*block)["data"]["protobuf"] = NAlice::ProtoToBase64String(directive);
    return block;
}

TContext::TBlock* TContext::AddRawServerDirective(NAlice::NScenarios::TServerDirective&& directive) {
    TBlock* block = Block();
    (*block)["type"].SetString(BLOCK_TYPE_UNIPROXY_ACTION);
    (*block)["command_type"].SetString("raw_server_directive");
    (*block)["data"]["protobuf"] = NAlice::ProtoToBase64String(directive);
    return block;
}

TContext::TBlock* TContext::AddCommand(TStringBuf type, const TDirectiveFactory::TDirectiveIndex& analyticsDirective, NSc::TValue value) {
    const auto analyticsTag = TDirectiveFactory::Get()->GetAnalyticsTag(analyticsDirective);
    return AddCommandImpl("command" /* type */, type /* commandType */, analyticsTag /* analyticsTag */, value /* data */);
}

TContext::TBlock* TContext::AddTypedSemanticFrame(const NAlice::TTypedSemanticFrame& tsf, const NAlice::TAnalyticsTrackingModule& atm) {
    TBlock* block = Block();
    (*block)["type"].SetString("typed_semantic_frame");
    (*block)["payload"] = NAlice::JsonStringFromProto(tsf);
    (*block)["analytics"] = NAlice::JsonStringFromProto(atm);
    return block;
}

TContext::TBlock* TContext::AddStackEngine(const NAlice::NScenarios::TStackEngine& stackEngine) {
    const TString jsonStackEngine = NAlice::JsonStringFromProto(stackEngine);
    auto data = NSc::TValue::FromJson(jsonStackEngine);

    TBlock* block = Block();
    (*block)["type"].SetString("stack_engine");
    (*block)["data"].Swap(data);
    return block;
}

TContext::TBlock* TContext::AddUniProxyAction(TStringBuf type, NSc::TValue value) {
    return AddCommandImpl(BLOCK_TYPE_UNIPROXY_ACTION /* type */, type /* commandType */, value /* data */);
}

TContext::TBlock* TContext::GetAttention(TStringBuf type) {
    for (auto& block : BlockList) {
        if ((*block)["type"] == TStringBuf("attention") && (*block)["attention_type"] == type) {
            return block.get();
        }
    }
    return nullptr;
}

TContext::TBlock* TContext::GetCommand(TStringBuf type) {
    for (auto& block : BlockList) {
        if ((*block)["type"] == TStringBuf("command") && (*block)["command_type"] == type) {
            return block.get();
        }
    }
    return nullptr;
}

TContext::TBlock* TContext::GetStats() {
    if (!StatsBlock) {
        StatsBlock = Block();
        (*StatsBlock)["type"].SetString("stats");
    }
    return StatsBlock;
}

TContext::TBlock* TContext::AddStatsCounter(TStringBuf name, i64 value) {
    auto* statsBlock = GetStats();
    (*statsBlock)["data"][name].SetIntNumber(value);
    return statsBlock;
}

TContext::TBlock* TContext::AddPlayerFeaturesBlock(const bool restorePlayer, const ui64 lastPlayTimestampMillis) {
    auto& block = *Block();
    block["type"] = "player_features";
    block["restore_player"] = restorePlayer;
    block["last_play_timestamp"] = lastPlayTimestampMillis;
    return &block;
}

NSc::TValue TContext::DeleteCommand(TStringBuf type) {
    return DeleteBlockImpl(TStringBuf("command"), type);
}

TContext::TBlock* TContext::AddSilentResponse(NSc::TValue value) {
    TBlock* block = Block();
    (*block)["type"].SetString("silent_response");
    (*block)["data"].Swap(value);
    return block;
}

TContext::TBlock* TContext::AddDivCardBlock(TStringBuf type, NSc::TValue data) {
    TBlock* divCard = Block();
    (*divCard)["type"].SetString("div_card");
    (*divCard)["card_template"].SetString(type);
    (*divCard)["data"].Swap(data);
    return divCard;
}

TContext::TBlock* TContext::AddDiv2CardBlock(TDiv2BlockBuilder builder) {
    TBlock* divCard = Block();
    (std::move(builder)).ToBlock(*divCard);
    return divCard;
}

TContext::TBlock* TContext::AddPreRenderedDivCardBlock(NSc::TValue data) {
    TBlock* divCard = Block();
    (*divCard)["type"].SetString("div_card");
    (*divCard)["card_layout"].Swap(data);
    return divCard;
}

TContext::TBlock* TContext::AddStopListeningBlock() {
    TBlock* block = Block();
    (*block)["type"].SetString("stop_listening");
    return block;
}

TContext::TBlock* TContext::AddVideoFactorsBlock(NSc::TValue data) {
    TBlock* block = Block();
    (*block)["type"].SetString(NAlice::NVideoCommon::VIDEO_FACTORS_BLOCK_TYPE);
    (*block)["data"].Swap(data);
    return block;
}

TContext::TBlock* TContext::AddTextCardBlock(TStringBuf id, NSc::TValue data) {
    TContext::TBlock* block = Block();
    (*block)["type"].SetString("text_card");
    (*block)["phrase_id"].SetString(id);
    (*block)["data"].Swap(data);
    return block;
}

TContext::TBlock* TContext::AddClientFeaturesBlock() {
    if (!ClientFeaturesBlock) {
        auto* blockHolderPtr = FindIfPtr(BlockList, [&](const auto& block) {
            return block->Get("type").GetString() == TStringBuf("client_features");
        });

        if (blockHolderPtr == nullptr) {
            ClientFeaturesBlock = Block();
            (*ClientFeaturesBlock)["type"].SetString("client_features");
        } else {
            ClientFeaturesBlock = blockHolderPtr->get();
        }
    }

    NSc::TValue& data = (*ClientFeaturesBlock)["data"].SetDict();
    ClientFeatures().ToJson(&data);

    return ClientFeaturesBlock;
}

TContext::TBlock* TContext::AddSpecialButtonBlock(TStringBuf type, NSc::TValue data) {
    TBlock* block = Block();
    (*block)["type"].SetString("special_button");
    (*block)["button_type"].SetString(type);
    (*block)["data"].Swap(data);
    return block;
}

TContext::TBlock* TContext::AddAutoactionDelayMsBlock(int delayMs) {
    TBlock* block = Block();
    (*block)["type"].SetString("autoaction_delay_ms");
    (*block)["delay_ms"].SetIntNumber(delayMs);
    return block;
}

TContext::TBlock* TContext::AddErrorBlock(const TError& error, const NSc::TValue& data) {
    TContext::TBlock* block = Block();

    error.ToJson(*block);
    (*block)["type"].SetString("error");
    (*block)["data"] = data;

    return block;
}

TContext::TBlock* TContext::AddErrorBlock(const TError& error) {
    TContext::TBlock* block = Block();

    error.ToJson(*block);
    (*block)["type"].SetString("error");
    (*block)["data"].SetNull();

    return block;
}

TContext::TBlock* TContext::AddErrorBlock(TError::EType type) {
    return AddErrorBlock(TError(type));
}

TContext::TBlock* TContext::AddErrorBlock(TError::EType type, TStringBuf msg) {
    return AddErrorBlock(TError(type, TString{msg}));
}

// Made as a separate function instead of just AddErrorBlock(const TError& error, TStringBuf code)
// because `ctx.AddErrorBlock(TError(), "some text")`-like calls became ambiguous
TContext::TBlock* TContext::AddErrorBlockWithCode(const TError& error, TStringBuf code) {
    NSc::TValue errData;
    errData["code"].SetString(code);
    return AddErrorBlock(error, std::move(errData));
}

TContext::TBlock* TContext::AddErrorBlockWithCode(TError::EType type, TStringBuf code) {
    return AddErrorBlockWithCode(TError(type), code);
}

bool TContext::HasAnyErrorBlock() const {
    return HasAnyBlockOfType(TStringBuf("error"));
}

TVector<NSc::TValue> TContext::DeleteErrorBlocks(const TVector<TError::EType>& types) {
    TVector<NSc::TValue> deleted;
    for (int i = BlockList.size() - 1; i >= 0; --i) {
        auto& block = *(BlockList[i]);
        if (block["type"] == TStringBuf("error")) {
            for (const auto& type : types) {
                if (block["error"]["type"] == ToString(type)) {
                    deleted.push_back(block);
                    BlockList.erase(BlockList.begin() + i);
                    break;
                }
            }
        }
    }
    return deleted;
}

TContext::TBlock* TContext::AddCommitCandidateBlock(NSc::TValue data) {
    TBlock* block = Block();
    (*block)["type"].SetString("commit_candidate");
    (*block)["data"].Swap(data);
    return block;
}

const TContext::TMeta& TContext::Meta() const {
    return RequestMetaScheme;
}

bool TContext::HasExpFlag(TStringBuf name) const {
    return Features.HasExpFlag(name);
}

TMaybe<TString> TContext::ExpFlag(TStringBuf name) const {
    return Features.Experiments().Value(name);
}

void TContext::OnEachExpFlag(const std::function<void(TStringBuf)>& fn) const {
    Features.Experiments().OnEachFlag(fn);
}

NAlice::TRawExpFlags TContext::ExpFlags() const {
    return Features.Experiments().GetRawExpFlags();
}

void TContext::UpdateLoggingReqInfo() const {
    if (SpeechKitEvent.Defined()) {
        TLogging::ReqInfo.Get().Update(RequestId, SpeechKitEvent->GetHypothesisNumber());
    } else {
        TLogging::ReqInfo.Get().Update(RequestId, -1 /* hypothesys number */);
    }
}

bool TContext::IsProviderTokenSet(TStringBuf name) const {
    return AuthTokens.find(name) != AuthTokens.end();
}

void TContext::SetProviderToken(TStringBuf name, TMaybe<TString> token) {
    AuthTokens[name] = std::move(token);
}

const TMaybe<TString> TContext::GetProviderToken(TStringBuf name) const {
    if (const auto* token = AuthTokens.FindPtr(name)) {
        return *token;
    }
    return Nothing();
}

TMaybe<NAlice::TBlackBoxHttpFetcher>& TContext::GetBlackBoxRequest() {
    return BlackBoxRequest;
}

void TContext::SetBlackBoxRequest(NAlice::TBlackBoxHttpFetcher&& request) {
    BlackBoxRequest.ConstructInPlace(std::move(request));
}

bool TContext::IsClassifiedAsChildRequest() const {
    if (!Meta().HasBiometryClassification()) {
        return false;
    }
    const auto& biometryClassification = Meta().BiometryClassification();
    for (const auto& classification : biometryClassification.Simple()) {
        if (classification.Tag() == "children" && classification.Classname() == "child") {
            return true;
        }
    }
    return false;
}

NAlice::EContentSettings TContext::CalculateContentRestrictionLevel() const {
    const auto& deviceConfig = Meta().DeviceState().DeviceConfig();
    TMaybe<ui32> filtrationLevel = Meta().HasFiltrationLevel() ? TMaybe<ui32>(Meta().FiltrationLevel()) : Nothing();

    if (GetIsClassifiedAsChildRequest()) {
        if (deviceConfig.HasChildContentSettings()) {
            const auto& childContentSettings = deviceConfig.ChildContentSettings();
            return NAlice::CalculateContentRestrictionLevel(ParseContentSettings(childContentSettings),
                                                            filtrationLevel);
        }
        return NAlice::EContentSettings::children;
    }

    const auto& contentSettings = deviceConfig.ContentSettings();
    return NAlice::CalculateContentRestrictionLevel(ParseContentSettings(contentSettings), filtrationLevel);
}

EContentRestrictionLevel TContext::GetContentRestrictionLevel() const {
    return NAlice::GetContentRestrictionLevel(ContentRestrictionLevel());
}

const NAlice::TUserLocation& TContext::UserLocation() {
    if (!UserLocation_) {
        const auto& meta = Meta();
        TString deviceTimeZone = meta.HasTimeZone() ? TString{*meta.TimeZone()} : TString();

        NGeobase::TId userRegion = NGeobase::UNKNOWN_REGION;
        if (meta.HasRegionId() && meta.RegionId() > 0) {
            userRegion = meta.RegionId();
        } else if (meta.HasLocation()) {
            // XXX check if lltogeo logging error (if not log it here)
            LLToGeo(*this, meta.Location().Lat(), meta.Location().Lon(), &userRegion);
        }

        UserLocation_ = NAlice::TUserLocation(
            GlobalCtx().GeobaseLookup(), userRegion, deviceTimeZone
        );
    }

    return *UserLocation_;
}

const TMaybe<NAlice::TUserLocation>& TContext::UserLocation() const {
    return UserLocation_;
}

NGeobase::TId TContext::UserRegion() {
    if (!UserLocation_) {
        UserLocation();
    }
    return UserLocation_->UserRegion();
}

TString TContext::UserTimeZone() {
    if (!UserLocation_) {
        UserLocation();
    }
    return UserLocation_->UserTimeZone();
}

const TString& TContext::UserTld() {
    if (!UserLocation_) {
        UserLocation();
    }
    return UserLocation_->UserTld();
}

const TString& TContext::UserAuthorizationHeader() const {
    return AuthorizationHeader;
}

const TString& TContext::GetAppInfoHeader() const {
    return AppInfoHeader;
}

bool TContext::IsAuthorizedUser() const {
    return !AuthorizationHeader.empty();
}

bool TContext::IsTestUser() const {
    // VINS uses 'deadbeef' prefix for all test queries.
    return AsciiHasPrefixIgnoreCase(Meta().UUID(), "deadbeef");
}

const TString& TContext::UserIP() const {
    return UserIPAddress;
}

TString TContext::GetDeviceId() const {
    return TString{Meta().DeviceState().HasDeviceId() ? Meta().DeviceState().DeviceId() : Meta().DeviceId()};
}

TString TContext::GetDeviceModel() const {
    TString keyDeviceModel;
    TString requestDeviceModel{Meta().ClientInfo().DeviceModel()};
    if(requestDeviceModel == TStringBuf("Station")) {
        keyDeviceModel = TStringBuf("yandexstation");
    } else if (requestDeviceModel == TStringBuf("YandexModule-00002")) {
        keyDeviceModel = TStringBuf("yandexmodule");
    } else {
        keyDeviceModel = requestDeviceModel;
    }
    return keyDeviceModel;
}

const NSc::TValue& TContext::ReqWizard(TStringBuf text, NGeobase::TId lr, TCgiParameters cgi, TStringBuf wizclient) {
    const TString key = TStringBuilder() << wizclient << text << cgi.Print();
    NSc::TValue* theWizardData = ReqWizardData.FindPtr(key);
    if (!theWizardData) {
        theWizardData = &ReqWizardData.insert({key, NSc::Null()}).first->second;

        cgi.InsertEscaped(TStringBuf("lr"), ToString(lr));
        cgi.InsertEscaped(TStringBuf("text"), text);
        cgi.InsertEscaped(TStringBuf("format"), TStringBuf("json"));
        cgi.InsertEscaped(TStringBuf("wizclient"), wizclient);

        NHttpFetcher::TRequestPtr req = GetSources().ReqWizard().Request();
        req->AddCgiParams(cgi);

        LOG(DEBUG) << "reqwizard request: " << TLogging::AsCurl(req->Url(), "") << Endl;
        NHttpFetcher::TResponse::TRef resp = req->Fetch()->Wait();

        if (!resp->IsError()) {
            *theWizardData = NSc::TValue::FromJson(resp->Data);
        } else {
            LOG(ERR) << "Error during fetching reqwizard: " << resp->GetErrorText() << Endl;
        }
    }

    return *theWizardData;
}

TContext::TSlot* TContext::GetSlot(TStringBuf name, TStringBuf type) {
    const TContext::TSlot* slot = static_cast<const TContext*>(this)->GetSlot(name, type);
    return const_cast<TContext::TSlot*>(slot);
}

const TContext::TSlot* TContext::GetSlot(TStringBuf name, TStringBuf type) const {
    const auto it = SlotMap.find(name);
    if (SlotMap.cend() == it || (type && type != it->second->Type)) {
        return nullptr;
    }

    return it->second;
}

TVector<TSlot*> TContext::GetSlots() const {
    TVector<TSlot*> slots;
    slots.reserve(SlotMap.size());
    for (const auto it: SlotMap) {
        slots.push_back(it.second);
    }
    return slots;
}

TContext::TSlot* TContext::CreateSlot(TStringBuf name, TStringBuf type, bool optional, const NSc::TValue& value,
                                      const NSc::TValue& sourceText) {
    TSlot* slot = GetOrCreateSlot(name, type);

    slot->Type = type;
    slot->Value = value;
    slot->Optional = optional;
    slot->SourceText = sourceText;

    return slot;
}

TContext::TSlot* TContext::GetOrCreateSlot(TStringBuf name, TStringBuf type) {
    TSlotMap::const_iterator it = SlotMap.find(name);
    if (SlotMap.cend() == it) {
        TSlot* slot = SlotList.emplace_back(std::make_unique<TSlot>(name, type)).get();
        it = SlotMap.emplace(slot->Name, slot).first;
    }

    return it->second;
}

bool TContext::DeleteSlot(TStringBuf name) {
    TSlotMap::const_iterator itMap = SlotMap.find(name);
    if (SlotMap.cend() == itMap) {
        return false;
    }
    SlotMap.erase(itMap);

    for (TSlotList::const_iterator itList = SlotList.begin(); itList != SlotList.end(); ++itList) {
        if (name == itList->get()->Name) {
            SlotList.erase(itList);
            return true;
        }
    }

    return false;
}

TVector<TContext::TSlot*> TContext::Slots(TStringBuf type) {
    TVector<TSlot*> slots;

    for (auto& slot : SlotList) {
        if (slot->Type != type) {
            continue;
        }

        slots.push_back(slot.get());
    }

    return slots;
}

void TContext::ToJsonImpl(NSc::TValue* out, TJsonOut flags) const {
    NSc::TValue jsonSlots;
    if (const auto error = SlotsToJson(&jsonSlots)) {
        error->ToJson(*out);
        return;
    }

    if (flags & EJsonOut::ServerAction) {
        (*out)["name"].SetString("update_form");
        (*out)["type"].SetString("server_action");
        out = &(*out)["payload"];
    }

    NSc::TValue* form = nullptr;
    if (flags & EJsonOut::TopLevel) {
        form = out;
    } else {
        if (flags & EJsonOut::FormUpdate) {
            form = &(*out)["form_update"];
        } else if (HasForm()) {
            form = &(*out)["form"];
        }
    }

    BlocksToJson(out);

    if (form) {
        (*form)["name"].SetString(FormName());
        (*form)["slots"] = std::move(jsonSlots);
        if (DontResubmit) {
            (*form)["dont_resubmit"] = DontResubmit;
        }

        if (flags & EJsonOut::Resubmit) {
            (*out)["resubmit"].SetBool(true);
        }
    }

    if (!RequestSessionStateScheme.IsNull()) {
        (*out)["session_state"] = *RequestSessionStateScheme.GetRawValue();
    }

    if (flags & EJsonOut::DataSources && !ReqDataSources.empty()) {
        auto& dataSources = (*out)["data_sources"];
        for (const auto& [key, value] : ReqDataSources) {
            dataSources[ToString(key)] = value;
        }
    }
}

NSc::TValue TContext::TopLevelToJson(TJsonOut flags) const {
    NSc::TValue out;
    ToJsonImpl(&out, flags);
    return out;
}

void TContext::ToJson(NSc::TValue* out, TJsonOut flags) const {
    if (ResponseForm) {
        ResponseForm->ToJson(out, flags);
    } else {
        ToJsonImpl(out, flags);
    }
}

NSc::TValue TContext::ToJson(TJsonOut flags) const {
    NSc::TValue out;
    ToJson(&out, flags);
    return out;
}

TSavedAddress TContext::GetSavedAddress(TSpecialLocation name, TStringBuf searchText) {
    if (!name.IsUserAddress()) {
        return TSavedAddress();
    }

    TAddressMap::const_iterator knownAddress = UserAddresses.find(name);
    if (knownAddress != UserAddresses.cend()) {
        return knownAddress->second;
    }

    TPersonalDataHelper personalData(*this);
    TStringBuf addressId(name);

    TSavedAddress userAddress;
    if (ClientFeatures().SupportsNavigator()) {
        userAddress = personalData.GetNavigatorUserAddress(addressId, searchText);
    } else {
        userAddress = personalData.GetDataSyncUserAddress(addressId);
    }

    if (userAddress.IsValid()) {
        UserAddresses.insert({name, userAddress});
    }

    return userAddress;
}

TResultValue TContext::DeleteSavedAddress(TSpecialLocation name) {
    if (!name.IsUserAddress()) {
        return TError(TError::EType::INVALIDPARAM, "Error while deleting address: invalid address name");
    }

    UserAddresses.erase(name);

    return TPersonalDataHelper(*this).DeleteDataSyncUserAddress(name);
}

TResultValue TContext::SaveAddress(TSpecialLocation name, const NSc::TValue& address) {
    if (!name.IsUserAddress()) {
        return TError(TError::EType::INVALIDPARAM, "Error while saving address: invalid address name");
    }

    TStringBuf addressId(name);
    TSavedAddress newAddress;
    if (TResultValue error = newAddress.FromGeo(addressId, address, Meta().Epoch())) {
        return error;
    }

    UserAddresses.insert({name, newAddress});

    TPersonalDataHelper personalData(*this);
    if (ClientFeatures().SupportsNavigator()) {
        return personalData.SaveNavigatorUserAddress(addressId, newAddress);
    } else {
        return personalData.SaveDataSyncUserAddress(addressId, newAddress);
    }
}

TMaybe<TUserBookmarksHelper> TContext::GetUserBookmarksHelper() {
    if (!UserBookmarks) {
        UserBookmarks = TUserBookmarksHelper(*this);
    }
    return UserBookmarks;
}

NAlice::NScenarios::TBassAnalyticsInfoBuilder& TContext::GetAnalyticsInfoBuilder() {
    if (!AnalyticsInfoBuilder) {
        AnalyticsInfoBuilder = NAlice::NScenarios::TBassAnalyticsInfoBuilder(FormName());
    }
    return *AnalyticsInfoBuilder;
}

TContext::TPtr TContext::SetResponseForm(TStringBuf newFormName, bool setCurrentFormAsCallback) {
    if (Meta().SuppressFormChanges()) {
        LOG(ERR) << "Attempted to change the form to " << newFormName << " while form changes are suppressed" << Endl;
        return {};
    }

    if (ResponseForm) {
        LOG(WARNING) << "Double form switch! New target form name: " << newFormName << Endl;
        if (HasExpFlag(NAlice::NExperiments::EXP_DISABLE_MULTIPLE_CHANGE_FORMS)) {
            LOG(WARNING) << "Multiple change forms are disabled" << Endl;
            return {};
        }
    }

    if (ChangeFormCounter > /* to prevent infinite recursion */10) {
        LOG(WARNING) << "Threshold level level of change form recursion detected for form: " << newFormName << Endl;
        return {};
    }

    LOG(DEBUG) << "Switch response form to " << newFormName << " from " << FormName()
               << (setCurrentFormAsCallback ? " with " : " without ") << "callback" << Endl;
    Y_STATS_INC_COUNTER(
        TStringBuilder() << TStringBuf("change_form_from_") << FormName() << TStringBuf("_to_") << newFormName
    );

    ResponseForm.Reset(new TContext(*this, newFormName));

    if (setCurrentFormAsCallback) {
        NSc::TValue currentForm;
        ToJsonImpl(&currentForm, TJsonOut(0));

        TSlot* form = ResponseForm->CreateSlot("callback_form", "form");
        form->Value = std::move(currentForm);
    }

    // Copying update_datasync actions - side effects that should be applied on Uniproxy.
    for (const auto& b : BlockList) {
        if (b->Get("type").GetString() == BLOCK_TYPE_UNIPROXY_ACTION &&
            b->Get("command_type").GetString() == UNIPROXY_ACTION_UPDATE_DATASYNC)
        {
            TBlock* block = ResponseForm->Block();
            (*block) = b->Clone();
        }
    }

    return ResponseForm;
}

void TContext::CopySlotsFrom(const TContext& ctx, std::initializer_list<TStringBuf> il) {
    NBASSEvents::TSlotsCopied slotsCopied;
    slotsCopied.SetFrom(ctx.FormName());

    for (const TStringBuf slotName : il) {
        auto it = ctx.SlotMap.find(slotName);
        if (ctx.SlotMap.cend() != it) {
            const TSlot* slot = it->second;
            const TSlot* newSlot = CreateSlot(slotName, slot->Type, slot->Optional, slot->Value, slot->SourceText);
            slotsCopied.AddSlots(newSlot->ToJson().ToJson());
        }
    }
}

TContext::TPtr TContext::SetCallbackAsResponseForm() {
    TContext::TSlot* callbackSlot = GetSlot("callback_form");
    if (IsSlotEmpty(callbackSlot)) {
        return nullptr;
    }

    TFormScheme callbackForm(&callbackSlot->Value["form"]);
    if (!SetResponseForm(callbackForm.Name(), false)) {
        return nullptr;
    }

    for (const auto& slot : callbackForm.Slots()) {
        ResponseForm->CreateSlot(slot.Name(), slot.Type(), slot.Optional(), *slot.Value(), *slot.SourceText());
    }
    return ResponseForm;
}

TContext::TPtr TContext::SetCallbackAsResponseFormAndCopySlots() {
    TContext::TSlot* callbackSlot = GetSlot("callback_form");
    if (IsSlotEmpty(callbackSlot)) {
        return nullptr;
    }

    TFormScheme callbackForm(&callbackSlot->Value["form"]);
    if (!SetResponseForm(callbackForm.Name(), false)) {
        return nullptr;
    }

    for (const auto& slot : SlotList) {
        ResponseForm->CreateSlot(slot->Name, slot->Type, slot->Optional, slot->Value, slot->SourceText);
    }
    for (const auto& slot : callbackForm.Slots()) {
        ResponseForm->CreateSlot(slot.Name(), slot.Type(), slot.Optional(), *slot.Value(), *slot.SourceText());
    }
    for (const auto& b : BlockList) {
        TBlock* block = ResponseForm->Block();
        (*block) = b->Clone();
    }

    ResponseForm->CreateSlot(TStringBuf("this_form_is_callback"), TStringBuf("bool"), true, true);
    return ResponseForm;
}

TResultValue TContext::RunResponseFormHandler() {
    if (ResponseForm) {
        return TRequestHandler(ResponseForm).RunFormHandler();
    }

    return TError(TError::EType::SYSTEM, "Can not run response form handler: response form was not specified");
}

const TAvatar* TContext::Avatar(TStringBuf ns, TStringBuf name, TStringBuf suffix) const {
    const TAvatarsMap& map = GlobalCtx().AvatarsMap();
    return map.Get(ns, TStringBuilder() << name << '.' << MatchScreenScaleFactor(ALLOWED_SCREEN_SCALE_FACTORS_DEFAULT) << suffix);
}

void TContext::MarkSensitive() {
    GetSensitiveBlock();
}

void TContext::MarkSensitive(const TSlot& slot) {
    auto* block = GetSensitiveBlock();
    (*block)["data"]["slots"].Push(slot.Name);
}

void TContext::SendPushRequest(const TStringBuf service, const TStringBuf event, TMaybe<TStringBuf> yandexUid, NSc::TValue serviceData) {
    const TStringBuf deviceId = Meta().DeviceState().HasDeviceId() ? Meta().DeviceState().DeviceId() : Meta().DeviceId();
    TString uid;
    if (yandexUid.Defined()) {
        uid = yandexUid.GetRef();
    } else {
        TPersonalDataHelper(*this).GetUid(uid);
    }
    const NPushNotification::TCallbackDataSchemeHolder callbackData = NPushNotification::GenerateCallbackDataSchemeHolder(Meta().UUID(), deviceId, uid, Meta().ClientId());
    NPushNotification::TResult request = NPushNotification::GetRequestsLocal(GlobalCtx(), serviceData, ToString(service), ToString(event), callbackData);
    NPushNotification::SendPushUnsafe(request, NPushNotification::ONBOARDING_EVENT);
}

EPermissionType TContext::GetPermissionInfo(EClientPermission permission) const {
    const auto* value = Permissions.FindPtr(ToString(permission));
    if (!value) {
        return EPermissionType::Unknown;
    }
    return *value ? EPermissionType::Permitted : EPermissionType::Forbidden;
}

TContext::TBlock* TContext::AddCommandImpl(TStringBuf type, TStringBuf commandType, NSc::TValue data) {
    AddVideoCommandToAnalyticsInfo(*this, commandType, data);
    TBlock* block = Block();
    (*block)["type"].SetString(type);
    (*block)["command_type"].SetString(commandType);
    (*block)["data"].Swap(data);
    return block;
}

TContext::TBlock* TContext::AddCommandImpl(TStringBuf type, TStringBuf commandType, TStringBuf analyticsTag, NSc::TValue data, bool beforeTts) {
    AddVideoCommandToAnalyticsInfo(*this, commandType, data);
    TBlock* block = Block();
    (*block)["type"].SetString(type);
    (*block)["command_type"].SetString(commandType);
    (*block)["command_sub_type"].SetString(analyticsTag);
    (*block)["data"].Swap(data);

    if (beforeTts && ClientFeatures().SupportsTtsPlayPlaceholder() && commandType != COMMAND_TTS_PLAY_PLACEHOLDER_TYPE) {
        while (!DeleteCommand(COMMAND_TTS_PLAY_PLACEHOLDER_TYPE).IsNull()) {
            block = BlockList.back().get();
        }
        AddCommand<TTtsPlayPlaceholderDirective>(COMMAND_TTS_PLAY_PLACEHOLDER_TYPE, NSc::TValue().SetDict());
    }

    return block;
}

TContext::TBlock TContext::DeleteBlockImpl(TStringBuf type, TStringBuf subtype) {
    TString subtypeName = TString{type} + TStringBuf("_type");
    for (auto it = BlockList.begin(); it != BlockList.end(); ++it) {
        auto& block = **it;
        if (block["type"] == type && block[subtypeName] == subtype) {
            NSc::TValue v = block;
            BlockList.erase(it);
            return v;
        }
    }
    return NSc::Null();
}

TContext::TBlock* TContext::GetSensitiveBlock() {
    if (!SensitiveBlock) {
        SensitiveBlock = Block();
        (*SensitiveBlock)["type"].SetString("sensitive");
    }
    return SensitiveBlock;
}

// static
TResultValue TContext::FromJsonImpl(const NSc::TValue& request, TInitializer initData, TConstructor constructor,
                                    bool shouldValidate) {
    using TRequestSchemeConst = NBASSRequest::TRequestConst<TSchemeTraits>;

    if (request.IsNull()) {
        return TError(TError::EType::INVALIDPARAM, "null request");
    }

    const TRequestSchemeConst scheme(&request);
    if (shouldValidate) {
        TResultValue err;
        auto validateCb = [&err](TStringBuf key, TStringBuf errmsg) {
            if (!err) {
                err = TError(TError::EType::INVALIDPARAM);
            } else {
                err->Msg << TStringBuf("; ");
            }
            err->Msg << key << TStringBuf(": ") << errmsg;
        };
        if (!scheme.Validate("", false /* strict */, validateCb)) {
            return err;
        }
    }

    TContext::TSlotList slots;
    for (const auto& schemeSlot : scheme.Form().Slots()) {
        slots.emplace_back(std::make_unique<TSlot>(schemeSlot));
    }

    NSc::TValue meta = *scheme.Meta().GetRawValue();

    TInstant now = ConstructInstantFromTimestamp(initData.FakeTimeHeader).GetOrElse(TInstant::Now());
    MergeLocationData(meta, now);

    TMaybe<TInputAction> inputAction;
    if (scheme.HasAction()) {
        inputAction.ConstructInPlace(scheme.Action());
    }

    if (const NSc::TValue& cp = meta.Get("config_patch"); !cp.IsNull()) {
        initData.ConfigPatch.MergeUpdate(cp);
    }

    TSetupResponses setupResponses;
    for (const auto& pair : scheme.SetupResponses()->GetDict()) {
        setupResponses.insert(std::make_pair(TString{pair.first}, NHttpFetcher::ResponseFromJson(pair.second)));
    }

    NSc::TValue blocks = request["blocks"];

    TDataSources dataSources;
    if (request["data_sources"].IsDict()) {
        const auto& dataSourcesDict = request["data_sources"].GetDict();
        for (const auto& [rawType, dataSource] : dataSourcesDict) {
            dataSources[FromString<int>(rawType)] = dataSource;
        }
    }

    return constructor(scheme.Form().Name(), std::move(inputAction), std::move(slots), meta, std::move(initData),
                       *scheme.Session().GetRawValue(), std::move(setupResponses), std::move(blocks),
                       std::move(dataSources));
}

TResultValue TContext::SlotsToJson(NSc::TValue* formJson, TStringBuf slotsKey) const {
    TResultValue error;
    NSc::TValue jsonSlots;
    for (const auto& slot : SlotList) {
        if (!slot->ToJson(&jsonSlots.Push(), &error)) {
            return error;
        }
    }

    if (!jsonSlots.IsNull()) {
        if (slotsKey) {
            (*formJson)[slotsKey] = std::move(jsonSlots);
        } else {
            formJson->MergeUpdate(jsonSlots);
        }
    } else {
        if (slotsKey) {
            (*formJson)[slotsKey].SetArray();
        } else {
            formJson->SetArray();
        }
    }

    return Nothing();
}

void TContext::BlocksToJson(NSc::TValue* formJson) const {
    NSc::TValue jsonBlocks;

    for (const auto& block : BlockList) {
        jsonBlocks.Push(*block);
    }

    if (AnalyticsInfoBuilder) {
        auto& scenarioAnalyticsInfoBlock = jsonBlocks.Push();
        scenarioAnalyticsInfoBlock["type"].SetString("scenario_analytics_info");
        scenarioAnalyticsInfoBlock["data"].SetString(AnalyticsInfoBuilder->SerializeAsBase64());
    }

    if (!jsonBlocks.IsNull()) {
        (*formJson)["blocks"] = jsonBlocks;
    }
}

// TContext::TInitializer -----------------------------------------------------
TContext::TInitializer::TInitializer(const TContext& ctx)
    : GlobalCtx(ctx.GlobalContext)
    , ReqId(ctx.ReqId())
    , MarketReqId(ctx.MarketReqId())
    , AuthHeader(ctx.AuthorizationHeader)
    , UserTicketHeader(ctx.UserTicketHeader)
    , Id(ctx.FId)
    , SpeechKitEvent(ctx.GetSpeechKitEvent())
{
    if (ctx.Config) {
        ConfigPatch = ctx.Config->AsJson();
    }
}

TContext::TInitializer::TInitializer(TGlobalContextPtr globalCtx, TStringBuf reqId, TStringBuf marketReqId,
                                     TStringBuf authHeader, TStringBuf appInfoHeader, TStringBuf fakeTimeHeader,
                                     TMaybe<TString> userTicketHeader, TMaybe<NAlice::TEvent>&& speechKitEvent)
    : GlobalCtx(globalCtx)
    , ReqId(reqId)
    , MarketReqId(marketReqId)
    , AuthHeader(authHeader)
    , AppInfoHeader(appInfoHeader)
    , FakeTimeHeader(fakeTimeHeader)
    , UserTicketHeader(std::move(userTicketHeader))
    , SpeechKitEvent(speechKitEvent)
{
}

// TInputAction ---------------------------------------------------------------
TInputAction::TInputAction(const TActionConst& scheme)
    : Name(scheme.Name())
    , Data(*scheme.Data().Get()) {
}

// TSlot ----------------------------------------------------------------------
TSlot::TSlot(const TSchemeConst& scheme)
    : Name(scheme.Name())
    , Type(scheme.Type())
    , SourceText(*scheme.SourceText())
    , Optional(scheme.Optional())
    , Value(*scheme.Value()) {
}

TSlot::TSlot(TStringBuf name, TStringBuf type)
    : Name(name)
    , Type(type)
    , Optional(true) {
}

NSc::TValue TSlot::ToJson() const {
    NSc::TValue slot;
    if (Type) {
        slot["type"].SetString(Type);
    }
    slot["name"].SetString(Name);
    slot["optional"].SetBool(Optional);
    slot["value"] = Value;

    if (!SourceText.IsNull()) {
        slot["source_text"] = SourceText;
    }

    return slot;
}

NSc::TValue TSlot::ToJson(TResultValue* error) const {
    NSc::TValue slot = ToJson();

    TResultValue err;
    TSlot::TScheme slotScheme(&slot);
    slotScheme.SetDefault();
    auto cb = [&err](TStringBuf key, TStringBuf errmsg) {
        if (!err) {
            err = TError(TError::EType::INVALIDPARAM);
        } else {
            err->Msg << TStringBuf("; ");
        }
        err->Msg << key << TStringBuf(": ") << errmsg;
    };
    if (!slotScheme.Validate("", false, cb)) {
        if (error) {
            *error = err;
        }
        return NSc::Null();
    }

    return slot;
}

bool TSlot::ToJson(NSc::TValue* out, TResultValue* error) const {
    NSc::TValue json = ToJson(error);
    if (json.IsNull()) {
        return false;
    }

    out->Swap(json);

    return true;
}

void TSlot::Reset() {
    Value.SetNull();
    SourceText.SetNull();
    Optional = true;
}

// ----------------------------------------------------------------------------
bool IsSlotEmpty(const TContext::TSlot* slot) {
    return !slot || slot->Value.IsNull();
}

} // namespace NBASS
