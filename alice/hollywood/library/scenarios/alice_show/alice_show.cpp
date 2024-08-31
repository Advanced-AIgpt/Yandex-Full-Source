#include "alice_show.h"

#include <alice/hollywood/library/scenarios/alice_show/nlg/register.h>
#include <alice/hollywood/library/scenarios/alice_show/proto/config.pb.h>
#include <alice/hollywood/library/scenarios/alice_show/proto/fast_data.pb.h>
#include <alice/hollywood/library/scenarios/alice_show/proto/scenario_data.pb.h>
#include <alice/hollywood/library/scenarios/alice_show/proto/state.pb.h>

#include <alice/hollywood/library/scenarios/fast_command/common.h>
#include <alice/hollywood/library/scenarios/music/common.h>
#include <alice/hollywood/library/phrases/phrases.h>
#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/tags/tags.h>

#include <alice/protos/data/news_provider.pb.h>
#include <alice/protos/data/scenario/alice_show/selectors.pb.h>
#include <alice/protos/data/scenario/music/topic.pb.h>

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/geo/protos/user_location.pb.h>
#include <alice/library/experiments/experiments.h>
#include <alice/library/proto/protobuf.h>
#include <alice/library/proto_eval/proto_eval.h>

#include <alice/megamind/library/util/slot.h>
#include <alice/megamind/protos/blackbox/blackbox.pb.h>
#include <alice/megamind/protos/common/atm.pb.h>
#include <alice/megamind/protos/property/alice_show_profile.pb.h>
#include <alice/megamind/protos/property/property.pb.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/megamind/protos/scenarios/stack_engine.pb.h>

#include <library/cpp/iterator/mapped.h>
#include <library/cpp/protobuf/json/util.h>
#include <library/cpp/string_utils/url/url.h>
#include <library/cpp/timezone_conversion/convert.h>

#include <util/generic/algorithm.h>
#include <util/string/cast.h>
#include <util/string/join.h>

#include <google/protobuf/util/message_differencer.h>

#include <limits>

using namespace NAlice::NScenarios;
using namespace NAlice::NHollywood::NAliceShow;
namespace NMemento = ru::yandex::alice::memento::proto;

namespace NAlice::NHollywood {

namespace {

using EShowPart = TAliceShowState::EShowPart;
using EShowType = TAliceShowState::EShowType;
using TDayPart = NData::NAliceShow::TDayPart;
using TAge = NData::NAliceShow::TAge;

constexpr TStringBuf NLG_ALICE_SHOW = "alice_show";
constexpr TStringBuf RENDER_NEWS_SUGGEST = "news_suggest";
constexpr TStringBuf RENDER_NEWS_SUGGEST_ACCEPTED = "news_suggest_accepted";
constexpr TStringBuf RENDER_NEWS_SUGGEST_REJECTED = "news_suggest_rejected";
constexpr TStringBuf RENDER_OUTRO = "render_outro";
constexpr TStringBuf RENDER_SUBSCRIPTION_REQUIRED = "subscription_required";
constexpr TStringBuf RENDER_NOT_SUPPORTED = "not_supported";
constexpr TStringBuf RENDER_AMBIGUOUS_SHOW_TYPE = "ambiguous_show_type";
constexpr TStringBuf RENDER_CONTEXT = "render_context";

constexpr TStringBuf HARDCODED_SHOW_TYPE_EVENING = "evening";
constexpr TStringBuf HARDCODED_SHOW_TYPE_CHILDREN = "children";

constexpr TStringBuf GET_NEXT_SHOW_BLOCK_CALLBACK = "alice_show_get_next_show_block";
constexpr TStringBuf GET_PREV_SHOW_BLOCK_CALLBACK = "alice_show_get_prev_show_block";
constexpr TStringBuf CONTINUE_PART_CALLBACK = "alice_show_continue_part";

constexpr TStringBuf ALICE_SHOW_ACTIVATE_FRAME = "alice.alice_show.activate";
constexpr TStringBuf ALICE_SHOW_GOOD_EVENING_FRAME = "alice.alice_show.good_evening";
constexpr TStringBuf ALICE_SHOW_GOOD_MORNING_FRAME = "alice.alice_show.good_morning";
constexpr TStringBuf ALICE_SHOW_GOOD_NIGHT_FRAME = "alice.alice_show.good_night";
constexpr TStringBuf ACCEPT_SUGGESTED_NEWS_FRAME = "alice.proactivity.confirm";
constexpr TStringBuf REJECT_SUGGESTED_NEWS_FRAME = "alice.proactivity.decline";
constexpr TStringBuf SLOT_DAY_PART = "day_part";
constexpr TStringBuf SLOT_AGE = "age";

constexpr TStringBuf PLAYER_NEXT_FRAME = "personal_assistant.scenarios.player.next_track";
constexpr TStringBuf PLAYER_PREV_FRAME = "personal_assistant.scenarios.player.previous_track";
constexpr TStringBuf PLAYER_CONTINUE_FRAME = "personal_assistant.scenarios.player.continue";

constexpr TStringBuf PLAYLIST_OF_THE_DAY = "playlist_of_the_day";
constexpr TStringBuf PLAYLIST_OF_THE_EVENING = "dailyCalm";

constexpr int SECONDS_SINCE_START_THRESHOLD = 55 * 60;

template <typename T>
class TMutableHolder {
public:
    explicit TMutableHolder(const T& value)
        : Value(value)
    { }

    TMutableHolder(T&& value)
        : Value(std::move(value))
    { }

    const T& Get() const {
        return Value;
    }

    operator const T&() const {
        return Get();
    }

    T& Mutable() {
        Mutated_ = true;
        return Value;
    }

    bool Mutated() const {
        return Mutated_;
    }

private:
    T Value;
    bool Mutated_ = false;
};

template <typename TContainer>
const auto& Choice(const TContainer& seq, IRng& rng) {
    return seq[rng.RandomInteger(seq.size())];
}

template <typename TContainer, typename TProto>
void EraseProto(TContainer& seq, const TProto& proto) {
    EraseIf(seq, [&](const TProto& element) {
        return google::protobuf::util::MessageDifferencer::Equivalent(proto, element);
    });
}

template <typename TShowSelector>
typename TShowSelector::EValue ParseShowSelector(const TList<TSlot>* showSelectorSlots) {
    if (showSelectorSlots == nullptr || showSelectorSlots->empty()) {
        return TShowSelector::Undefined;
    }
    const auto& showSelectorValue = showSelectorSlots->front().Value.AsString();
    // If there are more than one show type slots and some of them differ - the show type is ambiguous
    for (const auto& showSelectorSlot : *showSelectorSlots) {
        if (showSelectorValue != showSelectorSlot.Value.AsString()) {
            return TShowSelector::Ambiguous;
        }
    }
    typename TShowSelector::EValue showSelector = TShowSelector::Undefined;
    TShowSelector::EValue_Parse(showSelectorValue, &showSelector);
    return showSelector;
}

TDayPart::EValue ParseDayPart(const TFrame& activateFrame) {
    return ParseShowSelector<TDayPart>(activateFrame.FindSlots(SLOT_DAY_PART));
}

TAge::EValue ParseAge(const TFrame& activateFrame) {
    return ParseShowSelector<TAge>(activateFrame.FindSlots(SLOT_AGE));
}

NData::TNewsProvider MakeNewsProvider(const TStringBuf source, const TStringBuf rubric = {}) {
    NData::TNewsProvider provider;
    provider.SetNewsSource(TString{source});
    provider.SetRubric(TString{rubric});
    return provider;
}

NData::TNewsProvider DefaultNewsProvider() {
    return MakeNewsProvider(NMusic::DEFAULT_MORNING_SHOW_NEWS_SOURCE, NMusic::DEFAULT_MORNING_SHOW_NEWS_RUBRIC);
}

const TVector<NData::TNewsProvider> NEWS_PROVIDERS_NO_RUBRICS = {
    MakeNewsProvider("d0f021cb-dtf"),
    MakeNewsProvider("43e0281d-gazeta-ru"),
    MakeNewsProvider("d4777f11-kommersantu"),
    MakeNewsProvider("e3a1395f-lenta-ru"),
    MakeNewsProvider("d0cb2ee9-life-ru"),
    MakeNewsProvider("fa58ac81-moskovskij-komsomolec"),
    MakeNewsProvider("a7fce137-rambler-novosti"),
    MakeNewsProvider("c16d4bd9-n-1"),
    MakeNewsProvider("35376ef1-ria-novosti"),
    MakeNewsProvider("de9af378-rbk"),
    MakeNewsProvider("bed86bf3-vc-ru"),
};

using TPhrase = TPhraseGroup::TPhrase;

[[nodiscard]] TString CombineUrl(const TStringBuf base, const TStringBuf link) {
    if (!base || GetSchemePrefixSize(link)) {
        return TString{link};
    }
    return TString::Join(RemoveFinalSlash(base), "/", link);
}

template <typename T>
[[nodiscard]] const T* FindPtr(const NProtoBuf::Map<TString, T>& cont, const TStringBuf name) {
    if (const auto it = cont.find(name); it != cont.end()) {
        return &it->second;
    }
    return nullptr;
}

class TAliceShowFastData : public IFastData {
public:
    explicit TAliceShowFastData(const TAliceShowFastDataProto& proto)
        : PhraseCollection(proto.GetPhrasesCorpus())
        , TagConditionCollection(proto.GetTagConditionsCorpus())
        , ImageBaseUrl(proto.GetImageBaseUrl())
        , ImageDirective(proto.GetImageDirective())
        , Config(proto.GetConfig())
    {
        Images.reserve(proto.ImagesSize());
        for (const auto& image : proto.GetImages()) {
            Images.try_emplace(image.GetId(), image.GetUris());
        }
    }

    const TPhraseCollection& GetPhraseCollection() const {
        return PhraseCollection;
    }

    const TTagConditionCollection& GetTagConditionCollection() const {
        return TagConditionCollection;
    }

    TString GetImageUrl(TStringBuf id, IRng& rng) const {
        if (const auto imageUrisPtr = Images.FindPtr(id); imageUrisPtr && !imageUrisPtr->empty()) {
            return CombineUrl(ImageBaseUrl, Choice(*imageUrisPtr, rng));
        }
        return {};
    }

    const NScenarios::TDrawLedScreenDirective& GetImageDirective() const {
        return ImageDirective;
    }

    const TActionPart* FindActionPart(const TStringBuf name) const {
        return FindPtr(Config.GetParts(), name);
    }

    const TAction* FindAction(const TStringBuf name) const {
        return FindPtr(Config.GetActions(), name);
    }

    const TActionQueue* FindEntryActions(const TStringBuf vertex) const {
        const auto graphNode = FindPtr(Config.GetGraph(), vertex);
        if (graphNode && graphNode->GetEntryActions().ActionsSize()) {
            return &graphNode->GetEntryActions();
        }
        return {};
    }

    const TString& GetInitialVertex() const {
        return Config.GetInitialVertex();
    }

    const TTransitionChoice* FindTransitionChoice(const TStringBuf vertex) const {
        return FindPtr(Config.GetGraph(), vertex);
    }

private:
    const TPhraseCollection PhraseCollection;
    const TTagConditionCollection TagConditionCollection;

    TString ImageBaseUrl;
    THashMap<TString, NProtoBuf::RepeatedPtrField<TString>> Images;
    NScenarios::TDrawLedScreenDirective ImageDirective;

    const NAliceShow::TConfig Config;
};

class TAliceShow {
private:
    TRTLogger& Logger;
    IRng& CtxRng;
    const TScenarioRunRequest RequestProto;
    const TScenarioRunRequestWrapper Request;
    TNlgWrapper NlgWrapper;
    TRunResponseBuilder Builder;
    TAliceShowState State;
    TAliceShowState::TStage& Stage;
    TMutableHolder<TMorningShowNewsConfig> NewsConfig;
    TMutableHolder<TMorningShowTopicsConfig> TopicsConfig;
    TMutableHolder<TMorningShowSkillsConfig> SkillsConfig;
    TMutableHolder<TAliceShowScenarioData> ScenarioData;
    TUserLocationProto UserLocation;
    TResponseBodyBuilder& BodyBuilder;
    const TNlgData NlgData;
    const std::shared_ptr<const TAliceShowFastData> FastData;
    mutable TProtoEvaluatorWithTraceLog Evaluator;
    mutable TTagEvaluator TagEvaluator;
    mutable TRng Rng;

    using TActionPartModifierMethod = void (TAliceShow::*)(TActionPart&);
    struct TActionPartModifier {
        TActionPartModifierMethod Method;
        TTypedSemanticFrame::TypeCase RequiredFrame;
    };
    static const THashMap<TStringBuf, TActionPartModifier> ActionPartModifierMethods;

    using TPreRenderActionMethod = bool (TAliceShow::*)() const;
    static const THashMap<TStringBuf, TPreRenderActionMethod> PreRenderActions;

    using TPostRenderActionMethod = void (TAliceShow::*)();
    static const THashMap<TStringBuf, TPostRenderActionMethod> PostRenderActions;

public:
    explicit TAliceShow(TScenarioHandleContext& ctx)
        : Logger(ctx.Ctx.Logger())
        , CtxRng(ctx.Rng)
        , RequestProto(GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM))
        , Request(RequestProto, ctx.ServiceCtx)
        , NlgWrapper(TNlgWrapper::Create(ctx.Ctx.Nlg(), Request, ctx.Rng, ctx.UserLang))
        , Builder(&NlgWrapper)
        , State(GetAliceShowState())
        , Stage(*State.MutableStage())
        , NewsConfig(Request.BaseRequestProto().GetMemento().GetUserConfigs().GetMorningShowNewsConfig())
        , TopicsConfig(Request.BaseRequestProto().GetMemento().GetUserConfigs().GetMorningShowTopicsConfig())
        , SkillsConfig(Request.BaseRequestProto().GetMemento().GetUserConfigs().GetMorningShowSkillsConfig())
        , ScenarioData(GetScenarioData(Request))
        , UserLocation(GetUserLocation(Request).BuildProto())
        , BodyBuilder(CreateResponseBodyBuilder())
        , NlgData(Logger, Request)
        , FastData(GetScenarioFastData(ctx.Ctx.GlobalContext().FastData()))
        , Evaluator(Logger)
        , TagEvaluator(FastData->GetTagConditionCollection(), Evaluator)
        , Rng(Stage.GetRngSeed())
    {
        SetupEvaluator(Evaluator);
    }

    std::unique_ptr<TScenarioRunResponse> MakeResponse() && {
        Run();
        return ReleaseResponse();
    }

private:
    std::shared_ptr<const TAliceShowFastData> GetScenarioFastData(TFastData& fastData) const {
        LOG_INFO(Logger) << "FastData version " << fastData.GetVersion();
        return fastData.GetFastData<TAliceShowFastData>();
    }

    NProtoBuf::RepeatedPtrField<NScenarios::TDirective>& Directives() const {
        return *BodyBuilder.GetResponseBody().MutableLayout()->MutableDirectives();
    }

    TDrawLedScreenDirective::TDrawItem& CreateDrawLedScreenDirectiveItem() const {
        auto& directive = *Directives().Add()->MutableDrawLedScreenDirective();
        directive.CopyFrom(FastData->GetImageDirective());  // use directive template
        auto& items = *directive.MutableDrawItem();
        return items.empty() ? *items.Add() : items[0];
    }

    TTtsPlayPlaceholderDirective& CreateTtsPlayPlaceholderDirective() const {
        return *Directives().Add()->MutableTtsPlayPlaceholderDirective();
    }

    TListenDirective& CreateListenDirective() const {
        return *Directives().Add()->MutableListenDirective();
    }

    void AddImage(const TStringBuf id) const {
        if (!Request.Interfaces().GetHasLedDisplay()) {
            return;
        }
        if (TString imageUrl = FastData->GetImageUrl(id, Rng)) {
            CreateDrawLedScreenDirectiveItem().SetFrontalLedImage(std::move(imageUrl));
        }
    }

    ui64 GetExperimentValue(const TStringBuf prefix, ui64 defaultValue = 0) const {
        return FromStringWithDefault<ui64>(
            GetExperimentValueWithPrefix(
                Request.ExpFlags(),
                prefix
            ).GetOrElse(""),
            defaultValue
        );
    }

    bool PreRenderChooseNews() const {
        if (State.GetActivateFrame().GetAliceShowActivateSemanticFrame().HasNewsProvider()) {
            return false;
        }

        const ui64 suggestRejectionLimit = GetExperimentValue(
            EXP_HW_ALICE_SHOW_INTERACTIVITY_REJECTION_LIMIT_PREFIX,
            /* defaultValue = */ std::numeric_limits<ui64>::max()
        );
        const ui32 suggestRejectionStreak = ScenarioData.Get().GetSuggestRejectionStreak();
        if (suggestRejectionStreak >= suggestRejectionLimit) {
            return false;
        }

        const auto suggestCooldownDays = TDuration::Days(
            GetExperimentValue(EXP_HW_ALICE_SHOW_INTERACTIVITY_COOLDOWN_PREFIX)
        );
        const auto lastSuggestDatetime = TInstant::Seconds(ScenarioData.Get().GetLastSuggestTimestamp());
        const auto localDatetime = TInstant::Seconds(GetEpoch());
        return lastSuggestDatetime + suggestCooldownDays <= localDatetime;
    }

    const TTypedSemanticFrame* FindActivateSemanticFrame() const {
        const auto framePtr = FindSemanticFrame(ALICE_SHOW_ACTIVATE_FRAME) ?: FindSemanticFrame(ALICE_SHOW_GOOD_MORNING_FRAME) ?:
                              FindSemanticFrame(ALICE_SHOW_GOOD_EVENING_FRAME) ?: FindSemanticFrame(ALICE_SHOW_GOOD_NIGHT_FRAME);
        return framePtr ? &framePtr->GetTypedSemanticFrame() : nullptr;
    }

    bool IsNewSessionFrame() const {
        return FindActivateSemanticFrame() != nullptr;
    }

    TAliceShowState GetAliceShowState() {
        TAliceShowState state;
        if (!IsNewSessionFrame()) {
            const auto& rawState = Request.BaseRequestProto().GetState();
            if (rawState.UnpackTo(&state)) {
                LOG_INFO(Logger) << "AliceShow state was found; State is " << state;
            } else {
                LOG_INFO(Logger) << "AliceShow state was not found";
            }
        }
        auto& stage = *state.MutableStage();
        if (stage.GetShowIndex() == 0) {
            const bool ok = FillStateFromRequest(state);
            LOG_INFO(Logger) << "Selected show type: " << TAliceShowState::EShowType_Name(state.GetShowType())
                             << ", day part: " << TDayPart::EValue_Name(state.GetDayPart().GetValue())
                             << ", age: " << TAge::EValue_Name(state.GetAge().GetValue())
                             << (ok ? "" : ", configuration disabled");
            if (!ok) {
                SetIrrelevant();
            }
            stage.SetRngSeed(CtxRng.RandomInteger());
        }
        return state;
    }

    TResponseBodyBuilder& CreateResponseBodyBuilder() {
        return Builder.CreateResponseBodyBuilder();
    }

    void AddStopDirectives(const std::initializer_list<TStringBuf> directives = {PLAYER_PAUSE_DIRECTIVE, AUDIO_STOP_DIRECTIVE}) {
        const auto& deviceState = Request.BaseRequestProto().GetDeviceState();

        NJson::TJsonValue directiveValue;
        AddMultiroomSessionIdToDirectiveValue(directiveValue, deviceState);

        const TString analyticsType{GC_PAUSE_COMMAND};
        for (const auto directive : directives) {
            BodyBuilder.AddClientActionDirective(TString{directive}, analyticsType, directiveValue);
        }
    }

    void AddThinPlayerStopDirectives() {
        AddStopDirectives({AUDIO_STOP_DIRECTIVE});
    }

    void SetOnboarded(bool onboarded) {
        if (ScenarioData.Get().GetOnboarded() == onboarded) {
            return;
        }
        ScenarioData.Mutable().SetOnboarded(onboarded);
    }

    TStringBuf GetForcedEmotion() const {
        return GetExperimentValueWithPrefix(Request.ExpFlags(), EXP_HW_ALICE_SHOW_FORCED_EMOTION_PREFIX).GetOrElse("");
    }

    static TTypedSemanticFrame& AddTypedSemanticFrame(TResetAddBuilder& resetAddBuilder, TStringBuf purpose, TStringBuf forcedEmotion) {
        auto& utterance = resetAddBuilder.AddUtterance({}, EShouldListen::ForcedNo, TDirectiveChannel::Content, forcedEmotion);
        auto& analytics = *utterance.MutableAnalytics();
        analytics.SetPurpose(TString{purpose});
        analytics.SetOrigin(TAnalyticsTrackingModule::Scenario);
        return *utterance.MutableTypedSemanticFrame();
    }

    TTypedSemanticFrame& AddTypedSemanticFrame(TResetAddBuilder& resetAddBuilder, TStringBuf purpose) const {
        return AddTypedSemanticFrame(resetAddBuilder, purpose, GetForcedEmotion());
    }

    TTypedSemanticFrame& AddTypedSemanticFrame(TStringBuf purpose) {
        auto resetAddBuilder = BodyBuilder.ResetAddBuilder();
        return AddTypedSemanticFrame(resetAddBuilder, purpose);
    }

    static void AddCallback(TResetAddBuilder& resetAddBuilder, TStringBuf forcedEmotion, TStringBuf name) {
        auto& callback = resetAddBuilder.AddCallback(TString{name}, TDirectiveChannel::Content, forcedEmotion);
        callback.SetIsLedSilent(true);
    }

    void AddCallback(TResetAddBuilder& resetAddBuilder, TStringBuf name = GET_NEXT_SHOW_BLOCK_CALLBACK) const {
        AddCallback(resetAddBuilder, GetForcedEmotion(), name);
    }

    void AddCallback(TStringBuf name = GET_NEXT_SHOW_BLOCK_CALLBACK) {
        auto resetAddBuilder = BodyBuilder.ResetAddBuilder();
        AddCallback(resetAddBuilder, name);
    }

    TCallbackDirective* FindResetAddCallback() const {
        auto& body = BodyBuilder.GetResponseBody();
        if (!body.HasStackEngine()) {
            return {};
        }
        for (auto& action : *body.MutableStackEngine()->MutableActions()) {
            if (!action.HasResetAdd()) {
                continue;
            }
            for (auto& effect : *action.MutableResetAdd()->MutableEffects()) {
                if (effect.HasCallback()) {
                    return effect.MutableCallback();
                }
            }
        }
        return {};
    }

    NDatetime::TSimpleTM GetLocalTime() const {
        return NDatetime::ToCivilTime(TInstant::Seconds(GetEpoch()), NDatetime::GetTimeZone(Request.ClientInfo().Timezone));
    }

    static bool IsEveningTime(const NDatetime::TSimpleTM& localTime) {
        return localTime.Hour < 5 || localTime.Hour >= 18;
    }

    static bool IsGoodNightTime(const NDatetime::TSimpleTM& localTime) {
        return localTime.Hour < 5 || localTime.Hour >= 21;
    }

    bool IsChildTalking() const {
        return Request.BaseRequestProto().GetUserClassification().GetAge() == TUserClassification::Child;
    }

    TAge::EValue GetDefaultAgeForRequest() const {
        const bool forChildren = (Request.FiltrationMode() == TUserPreferences::Safe || IsChildTalking());
        return forChildren ? TAge::Children : TAge::Adult;
    }

    TDayPart::EValue GetDefaultDayPartForRequest() const {
        const auto localTime = GetLocalTime();
        if (IsGoodNightTime(localTime)) {
            return TDayPart::Night;
        }
        if (IsEveningTime(localTime)) {
            return TDayPart::Evening;
        }
        return TDayPart::Morning;
    }

    bool GetForChildren() const {
        return State.GetAge().GetValue() == TAge::Children;
    }

    bool FillState(TAliceShowState& state, TDayPart::EValue dayPart, TAge::EValue age = {}) const {
        if (!dayPart) {
            dayPart = GetDefaultDayPartForRequest();
        }
        if (!age) {
            age = GetDefaultAgeForRequest();
        }
        state.MutableDayPart()->SetValue(dayPart);
        state.MutableAge()->SetValue(age);
        state.SetShowType(GetShowTypeByDayPartAndAge(dayPart, age));
        return IsShowEnabled(dayPart, age);
    }

    static TAliceShowState::EShowType GetShowTypeByDayPartAndAge(TDayPart::EValue dayPart, TAge::EValue age) {
        const bool forChildren = (age == TAge::Children);
        switch (dayPart) {
            case TDayPart::Morning:
                return forChildren ? TAliceShowState::Children : TAliceShowState::Morning;
            case TDayPart::Evening:
                return forChildren ? TAliceShowState::Night : TAliceShowState::Evening;
            case TDayPart::Night:
                return TAliceShowState::Night;
            default:
                return TAliceShowState::Undefined;
        }
    }

    bool IsShowEnabled(TDayPart::EValue dayPart, TAge::EValue age) const {
        // Will be rejected later
        if (IsShowTypeAmbigious(dayPart, age)) {
            return true;
        }
        switch (dayPart) {
            case TDayPart::Morning:
            case TDayPart::Evening:
            case TDayPart::Night:
                return true;
            default:
                return false;
        }
    }

    bool IsShowTypeAmbigious(TDayPart::EValue dayPart, TAge::EValue age) const {
        return dayPart == TDayPart::Ambiguous || age == TAge::Ambiguous;
    }

    bool FillStateFromRequest(TAliceShowState& state) const {
        if (const auto activateFramePtr = FindActivateSemanticFrame()) {
            state.MutableActivateFrame()->CopyFrom(*activateFramePtr);
        }
        if (const auto activateFrameProto = FindSemanticFrame(ALICE_SHOW_ACTIVATE_FRAME)) {
            const auto activateFrame = TFrame::FromProto(*activateFrameProto);
            return FillState(state, ParseDayPart(activateFrame), ParseAge(activateFrame));
        }
        if (FindSemanticFrame(ALICE_SHOW_GOOD_MORNING_FRAME)) {
            return FillState(state, TDayPart::Morning);
        }
        if (FindSemanticFrame(ALICE_SHOW_GOOD_EVENING_FRAME)) {
            return FillState(state, TDayPart::Evening) &&
                   Request.HasExpFlag(EXP_HW_ENABLE_EVENING_SHOW_GOOD_EVENING);
        }
        if (FindSemanticFrame(ALICE_SHOW_GOOD_NIGHT_FRAME)) {
            return FillState(state, TDayPart::Night) &&
                   Request.HasExpFlag(EXP_HW_ENABLE_GOOD_NIGHT_SHOW_GOOD_NIGHT);
        }
        return false;
    }

    ui64 GetEpoch() const {
        return Request.ClientInfo().Epoch;
    }

    void SetIrrelevant() {
        Builder.SetIrrelevant();
    }

    const TPtrWrapper<TSemanticFrame> FindSemanticFrame(TStringBuf name) const {
        return Request.Input().FindSemanticFrame(name);
    }

    bool HasCallback(TStringBuf name) const {
        if (const auto callback = Request.Input().GetCallback()) {
            return callback->GetName() == name;
        }
        return false;
    }

    TAliceShowScenarioData GetScenarioData(const TScenarioRunRequestWrapper& request) const {
        TAliceShowScenarioData scenarioData;
        const auto& scenarioDataAny = request.BaseRequestProto().GetMemento().GetScenarioData();
        if (scenarioDataAny.UnpackTo(&scenarioData)) {
            LOG_INFO(Logger) << "Scenario data found: " << scenarioData;
        } else {
            LOG_INFO(Logger) << "Scenario data not found";
        }
        return scenarioData;
    }

    void SetupEvaluator(TProtoEvaluator& evaluator) const {
        const auto localTime = GetLocalTime();
        evaluator.SetParameterValue("Hour", ToString(localTime.Hour));
        evaluator.SetParameterValue("MinutesFromMidnight", ToString(localTime.Hour * 60u + localTime.Min));
        evaluator.SetParameterValue("MDay", ToString(localTime.MDay));
        evaluator.SetParameterValue("Month", ToString(localTime.RealMonth()));
        evaluator.SetParameterValue("Year", ToString(localTime.RealYear()));
        evaluator.SetProtoRef("NewsConfig", NewsConfig.Get());
        evaluator.SetProtoRef("Request", RequestProto);
        evaluator.SetProtoRef("ScenarioData", ScenarioData.Get());
        evaluator.SetProtoRef("SkillsConfig", SkillsConfig.Get());
        evaluator.SetProtoRef("Stage", Stage);
        evaluator.SetProtoRef("State", State);
        evaluator.SetProtoRef("TopicsConfig", TopicsConfig.Get());
        evaluator.SetProtoRef("UserLocation", UserLocation);
        if (const auto userInfoPtr = GetUserInfoProto(Request)) {
            evaluator.SetProtoRef("UserInfo", *userInfoPtr);
        }
        for (const auto& [exp, _] : Request.ExpFlags()) {
            TStringBuf name = exp;
            TStringBuf value = "1";
            name.TrySplit('=', name, value);
            evaluator.SetParameterValue(name, TString{value});
        }
    }

    static TString FormatTagStat(const THashMap<TString, size_t>& tagStat) {
        TString result;
        if (!tagStat.empty()) {
            TStringOutput tagStatLog{result};
            tagStatLog << " (tags";
            for (const auto& [k, v] : tagStat) {
                tagStatLog << ' ' << k << ':' << v;
            }
            tagStatLog << ")";
        }
        return result;
    }

    [[nodiscard]] TVector<const TPhrase*> FindPhrases(const TStringBuf groupName) const {
        TVector<const TPhrase*> result;
        THashMap<TString, size_t> tagStat;
        const auto tagChecker = [&](const TStringBuf tag) { return TagEvaluator.CheckTag(tag); };
        const auto consumer = [&](auto&& phrase, auto&& tags, double probability) {
            if (probability < 1 && CtxRng.RandomDouble() >= probability) {
                return false;
            }
            result.emplace_back(&phrase);
            if (!tags.empty()) {
                ++tagStat[JoinSeq("/", tags)];
            }
            return true;
        };
        FastData->GetPhraseCollection().FindPhrases(groupName, tagChecker, consumer);
        LOG_INFO(Logger) << "Collected " << result.size() << " phrases from " << groupName << FormatTagStat(tagStat);
        return result;
    }

    const TPhrase* GetPhrase(TStringBuf name) const {
        const auto phrases = FindPhrases(name);
        if (phrases.empty()) {
            Rng.RandomInteger();    // generate a random for consistency
            return nullptr;
        }
        return Choice(phrases, Rng);
    }

    void AddTtsPlaceholderIfNeeded() const {
        if (BodyBuilder.GetResponseBody().GetLayout().DirectivesSize()) {
            CreateTtsPlayPlaceholderDirective().SetDirectiveChannel(TDirectiveChannel::Content);
        }
    }

    TString SeasonedVoice(const TPhrase& phrase) const {
        const TString& voice = phrase.GetVoice() ?: phrase.GetText();
        if (Request.RequestSource() == TScenarioBaseRequest::GetNext) {
            // voice is seasoned by ResetAdd options
            return voice;
        }
        if (const auto emotion = GetForcedEmotion()) {
            return TString::Join("<speaker emotion=\"", emotion, "\">", voice);
        }
        return voice;
    }

    class TPhraseBuilder {
    public:
        TPhraseBuilder(TString text, TString voice)
            : Text(std::move(text))
            , Voice(std::move(voice))
        {}

        void AddPhrase(const TPhrase* phrase) {
            if (!phrase) {
                return;
            }
            Text.append(" ").append(phrase->GetText());
            Voice.append(" ").append(phrase->GetVoice() ?: phrase->GetText());
        }

        TNlgData CreateNlgData(TNlgData nlgData) {
            nlgData.Context["text"] = std::move(Text);
            nlgData.Context["voice"] = std::move(Voice);
            return nlgData;
        }

    private:
        TString Text;
        TString Voice;
    };

    bool TryRenderPhrase(TStringBuf name, const NProtoBuf::RepeatedPtrField<TString>& appendPhrases) const {
        if (const auto phrase = GetPhrase(name)) {
            TPhraseBuilder builder{phrase->GetText(), SeasonedVoice(*phrase)};
            for (const auto& appendPhrase : appendPhrases) {
                builder.AddPhrase(GetPhrase(appendPhrase));
            }
            BodyBuilder.AddRenderedTextAndVoice(NLG_ALICE_SHOW, RENDER_CONTEXT, builder.CreateNlgData(NlgData));
            AddTtsPlaceholderIfNeeded();
            return true;
        }
        return false;
    }

    void RenderPhrase(TStringBuf name, const NProtoBuf::RepeatedPtrField<TString>& append = {}) const {
        if (!TryRenderPhrase(name, append)) {
            // fallback to NLG
            BodyBuilder.AddRenderedTextAndVoice(NLG_ALICE_SHOW, name, NlgData);
            AddTtsPlaceholderIfNeeded();
        }
    }

    bool IsEveningShow() const {
        return State.GetDayPart().GetValue() == TDayPart::Evening;
    }

    bool CheckFlags(const NProtoBuf::RepeatedPtrField<TString>& flags) const {
        return AllOf(flags, [this](const TStringBuf flag) {
            return flag.StartsWith('!') ? !Request.HasExpFlag(flag.substr(1)) : Request.HasExpFlag(flag);
        });
    }

    bool CheckTags(const NProtoBuf::RepeatedPtrField<TString>& tags) const {
        return AllOf(tags, [&](const TStringBuf tag) { return TagEvaluator.CheckTag(tag); });
    }

    TActionPart BuildActionPartFromBase(const TActionPart& derivedPart, TSet<TString>& usedParts) {
        const auto& baseName = derivedPart.GetUsePart();
        if (!baseName) {
            return derivedPart;
        }
        auto [it, inserted] = usedParts.emplace(baseName);
        if (!inserted) {
            LOG_WARNING(Logger) << "Loop detected in UsePart references: " << baseName;
            return derivedPart;
        }
        if (const TActionPart* const basePartPtr = FastData->FindActionPart(baseName)) {
            auto part = BuildActionPart(*basePartPtr, usedParts);
            LOG_INFO(Logger) << "Used base ActionPart: " << baseName;
            part.MergeFrom(derivedPart);
            part.ClearUsePart();
            LOG_DEBUG(Logger) << "ActionPart after merge: " << part;
            return part;
        }
        LOG_WARNING(Logger) << "Unknown ActionPart referenced from UsePart: " << baseName;
        return derivedPart;
    }

    TActionPart BuildActionPart(const TActionPart& actionPartSource, TSet<TString>& usedParts) {
        auto part = BuildActionPartFromBase(actionPartSource, usedParts);
        for (const auto& modName : part.GetModifiers()) {
            if (const auto modPtr = ActionPartModifierMethods.FindPtr(modName)) {
                if (modPtr->RequiredFrame && part.GetSemanticFrame().GetTypeCase() != modPtr->RequiredFrame) {
                    LOG_WARNING(Logger) << "Modifier \"" << modName << "\" requires frame type "
                                        << static_cast<int>(modPtr->RequiredFrame) << " which is not present";
                } else {
                    // execute the modifier
                    (this->*(modPtr->Method))(part);
                }
            } else {
                LOG_WARNING(Logger) << "Unknown modifier: " << modName;
            }
        }
        part.ClearModifiers();
        return part;
    }

    TActionPart BuildActionPart(const TActionPart& actionPartSource) {
        TSet<TString> usedParts;
        return BuildActionPart(actionPartSource, usedParts);
    }

    bool RenderActionPart(const TActionPart& actionPartSource, TResetAddBuilder& resetAddBuilder) {
        bool haveContentAuto = false;
        auto part = BuildActionPart(actionPartSource);

        for (auto& nluHint : *part.MutableNluHints()) {
            BodyBuilder.AddNluHint(std::move(nluHint));
        }
        // image should go before TTS (for proper directive order)
        if (const auto& imageId = part.GetImageId()) {
            AddImage(imageId);
        }
        if (const auto& phraseId = part.GetPhraseId()) {
            RenderPhrase(phraseId, part.GetAppendPhrases());
            haveContentAuto = true;
        }
        // listen directive should go after TTS
        if (part.GetListen()) {
            CreateListenDirective();
        }
        if (part.HasSemanticFrame()) {
            AddTypedSemanticFrame(resetAddBuilder, part.GetPurpose()) = std::move(*part.MutableSemanticFrame());
            haveContentAuto = true;
        }
        return part.HasHaveContent() ? part.GetHaveContent().value() : haveContentAuto;
    }

    bool PreRenderAction(const TStringBuf name) const {
        if (const auto preRenderPtr = PreRenderActions.FindPtr(name)) {
            return (this->**preRenderPtr)();
        }
        return true;
    }

    void PostRenderAction(const TStringBuf name) {
        if (const auto postRenderPtr = PostRenderActions.FindPtr(name)) {
            (this->**postRenderPtr)();
        }
    }

    bool MayRenderAction(const TStringBuf name, const TAction& action) const {
        if (!PreRenderAction(name)) {
            LOG_INFO(Logger) << "Action \"" << name << "\" was disabled by pre-render hook";
            return false;
        }
        if (action.HasCondition() && !Evaluator.Evaluate<bool>(action.GetCondition())) {
            LOG_INFO(Logger) << "Action \"" << name << "\" was disabled by Condition";
            return false;
        }
        return true;
    }

    bool RenderAction(const TStringBuf name, const TAction& action) {
        const size_t partIndex = Stage.GetShowPartIndex();
        const size_t partsCount = action.ActionPartsSize();

        if (partIndex >= partsCount || !MayRenderAction(name, action)) {
            Stage.SetShowPartIndex(0);
            AddCallback();
            return false;
        }

        LOG_INFO(Logger) << "Rendering action \"" << name << "\" part " << partIndex + 1 << '/' << partsCount;

        const bool lastPart = (partIndex == partsCount - 1);
        auto resetAddBuilder = BodyBuilder.ResetAddBuilder();
        if (!lastPart) {
            AddCallback(resetAddBuilder, CONTINUE_PART_CALLBACK);
        } else if (!action.GetStop()) {
            AddCallback(resetAddBuilder);
        }
        bool haveContent = RenderActionPart(action.GetActionParts(partIndex), resetAddBuilder);
        PostRenderAction(name);
        if (Stage.GetHardcodedShowIndex() < action.GetNextHardcodedShowIndex()) {
            Stage.SetHardcodedShowIndex(action.GetNextHardcodedShowIndex());
        }
        Stage.SetShowPartIndex(lastPart ? 0 : partIndex + 1);
        return haveContent;
    }

    bool RenderAction(TStringBuf name) {
        if (const auto actionPtr = FastData->FindAction(name)) {
            return RenderAction(name, *actionPtr);
        }
        LOG_WARNING(Logger) << "Unknown action: " << name;
        RenderPhrase(RENDER_OUTRO);
        return false;
    }

    void FillPlan(const TStringBuf vertex) {
        auto& plan = *State.MutablePlan();
        if (plan.GetVertex() && plan.GetActionQueue().ActionsSize()) {
            *State.AddPlanHistory() = std::move(plan);
        }
        plan.Clear();
        plan.SetVertex(TString{vertex});
        if (const auto queuePtr = FastData->FindEntryActions(vertex)) {
            for (const auto& actionName : queuePtr->GetActions()) {
                if (const auto actionPtr = FastData->FindAction(actionName)) {
                    if (!CheckTags(actionPtr->GetTags())) {
                        LOG_INFO(Logger) << "Action \"" << actionName << "\" was disabled by tags";
                    } else if (!CheckFlags(actionPtr->GetFlags())) {
                        LOG_INFO(Logger) << "Action \"" << actionName << "\" was disabled by flags";
                    } else {
                        *plan.MutableActionQueue()->AddActions() = actionName;
                    }
                } else {
                    LOG_WARNING(Logger) << "Unknown action: " << actionName;
                }
            }
        }
        LOG_INFO(Logger) << "Filled plan: " << plan;
    }

    double GetTransitionWeight(const TTransition& transition) const {
        if (!transition.HasWeight()) {
            return 1;
        }
        try {
            return Evaluator.Evaluate<double>(transition.GetWeight());
        } catch (...) {
            LOG_ERROR(Logger) << "Exception in transition weight evaluation: " << CurrentExceptionMessage();
            return 0;
        }
    }

    TString ChooseNextVertex() const {
        const auto& currentVertex = State.GetPlan().GetVertex();
        if (!currentVertex) {
            return FastData->GetInitialVertex();
        }
        if (const auto choicePtr = FastData->FindTransitionChoice(currentVertex)) {
            struct TWeightedTransition {
                const TTransition& Transition;
                const double Weight;
            };
            TVector<TWeightedTransition> weightedTransitions;
            weightedTransitions.reserve(choicePtr->TransitionsSize());
            double weightSum = 0;
            for (const auto& transition : choicePtr->GetTransitions()) {
                if (!CheckFlags(transition.GetFlags()) || !CheckTags(transition.GetTags())) {
                    continue;
                }
                if (const double weight = GetTransitionWeight(transition); weight > 0) {
                    if (choicePtr->GetChoice() == TTransitionChoice::First || choicePtr->TransitionsSize() == 1) {
                        return transition.GetTargetVertex();
                    }
                    weightedTransitions.emplace_back(TWeightedTransition{transition, weight});
                    weightSum += weight;
                }
            }
            if (weightSum > 0) {
                // use CtxRng instead of Rng to avoid messing up rendering when navigating backwards
                const double r = CtxRng.RandomDouble(0, weightSum);
                weightSum = 0;
                for (const auto& wt : weightedTransitions) {
                    weightSum += wt.Weight;
                    if (r < weightSum) {
                        return wt.Transition.GetTargetVertex();
                    }
                }
            }
        }
        return {};
    }

    void TryAddPushDirective() {
        if (TagEvaluator.CheckTag("push")) {
            const auto pushes = ScenarioData.Get().GetPushesSent();
            NMusic::AddAliceShowPushDirective(pushes, CtxRng, BodyBuilder);
            ScenarioData.Mutable().SetPushesSent(pushes + 1);
            ScenarioData.Mutable().SetLastPushTimestamp(GetEpoch());
        }
    }

    void PostRenderGreeting() {
        TryAddPushDirective();
        SetOnboarded(true);
    }

    bool HasPhrase(TStringBuf name) const {
        return FastData->GetPhraseCollection().FindPhraseGroup(name) != nullptr;
    }

    TMaybe<TString> GetNewsPhrase(const NData::TNewsProvider& provider, const TStringBuf phrase) const {
        static constexpr TStringBuf sep = "__";
        if (const auto newsPhrase = TString::Join(phrase, sep, provider.GetNewsSource(), sep, provider.GetRubric()); HasPhrase(newsPhrase)) {
            return newsPhrase;
        }
        if (const auto newsPhrase = TString::Join(phrase, sep, provider.GetRubric()); IsDefaultNewsSource(provider) && HasPhrase(newsPhrase)) {
            return newsPhrase;
        }
        if (const auto newsPhrase = TString::Join(phrase, sep, provider.GetNewsSource()); HasPhrase(newsPhrase)) {
            return newsPhrase;
        }
        return Nothing();
    }

    bool HasNewsPhrase(const NData::TNewsProvider& provider, const TStringBuf phrase) const {
        return GetNewsPhrase(provider, phrase).Defined();
    }

    void EraseNewsProvidersWithoutNlg(google::protobuf::RepeatedPtrField<NData::TNewsProvider>& newsProviders) const {
        EraseIf(newsProviders, [&](const NData::TNewsProvider& provider) {
            return !HasNewsPhrase(provider, RENDER_NEWS_SUGGEST);
        });
    }

    NData::TNewsProvider SuggestNewsProvider() const {
        auto selectedNews = NewsConfig.Get().GetNewsProviders();
        EraseNewsProvidersWithoutNlg(selectedNews);
        const auto totalNewsSize = NewsConfig.Get().NewsProvidersSize();

        if (Request.HasExpFlag(EXP_HW_ALICE_SHOW_INTERACTIVITY_SELECTED_ONLY_PREFIX) && totalNewsSize > 1 && selectedNews.size() > 0) {
            return Choice(selectedNews, Rng);
        }
        auto newsSuggests = NEWS_PROVIDERS_NO_RUBRICS;
        // Don't suggest the only selected NewsProvider
        if (totalNewsSize == 1 && selectedNews.size() == 1) {
            EraseProto(newsSuggests, selectedNews[0]);
        }
        return Choice(newsSuggests, Rng);
    }

    void ModifierSuggestNews(TActionPart& part) {
        auto suggestedNewsProvider = SuggestNewsProvider();
        if (auto newsPhrase = GetNewsPhrase(suggestedNewsProvider, part.GetPhraseId()); newsPhrase.Defined()) {
            part.SetPhraseId(std::move(newsPhrase.GetRef()));
        }
        *State.MutableNewsSuggest()->MutableProvider() = std::move(suggestedNewsProvider);
        ScenarioData.Mutable().SetLastSuggestTimestamp(GetEpoch());
    }

    bool HasNewsSuggest() const {
        return State.HasNewsSuggest() && State.GetNewsSuggest().HasProvider();
    }

    bool TryAddNewsProvider(const NData::TNewsProvider& provider) {
        for (const auto& providerFromConfig : NewsConfig.Get().GetNewsProviders()) {
            if (google::protobuf::util::MessageDifferencer::Equivalent(provider, providerFromConfig)) {
                return false;
            }
        }
        *NewsConfig.Mutable().AddNewsProviders() = provider;
        NewsConfig.Mutable().SetDefault(false);
        return true;
    }

    void ModifierSuggestNewsFeedback(TActionPart& part) {
        if (HasNewsSuggest() && FindSemanticFrame(ACCEPT_SUGGESTED_NEWS_FRAME)) {
            part.SetPhraseId(TString{RENDER_NEWS_SUGGEST_ACCEPTED});
            part.SetImageId("happy");

            State.MutableNewsSuggest()->SetAccepted(true);

            TryAddNewsProvider(State.MutableNewsSuggest()->GetProvider());

            if (ScenarioData.Get().GetSuggestRejectionStreak() > 0) {
                ScenarioData.Mutable().SetSuggestRejectionStreak(0);
            }
        } else {
            part.SetPhraseId(TString{RENDER_NEWS_SUGGEST_REJECTED});
            part.SetImageId("neutral");

            State.MutableNewsSuggest()->SetAccepted(false);

            ScenarioData.Mutable().SetSuggestRejectionStreak(
                ScenarioData.Get().GetSuggestRejectionStreak() + 1
            );
        }
    }

    NData::TNewsProvider ChooseNewsProvider() const {
        const auto& activateFrame = State.GetActivateFrame().GetAliceShowActivateSemanticFrame();
        if (activateFrame.HasNewsProvider()) {
            return activateFrame.GetNewsProvider().GetNewsProviderValue();
        }
        if (HasNewsSuggest() && State.GetNewsSuggest().GetAccepted()) {
            return State.GetNewsSuggest().GetProvider();
        }
        auto newsProviders = NewsConfig.Get().GetNewsProviders();
        EraseIf(newsProviders, [&](const NData::TNewsProvider& provider) {
            return IsDefaultNewsSource(provider);
        });
        // Don't play rejected News
        if (HasNewsSuggest()) {
            EraseProto(newsProviders, State.GetNewsSuggest().GetProvider());
        }
        if (!newsProviders.empty()) {
            return Choice(newsProviders, CtxRng);
        }
        if (Request.HasExpFlag(EXP_ALICE_SHOW_NEWS_NO_RUBRICS)) {
            return Choice(NEWS_PROVIDERS_NO_RUBRICS, CtxRng);
        }
        return DefaultNewsProvider();
    }

    static bool IsDefaultNewsSource(const NData::TNewsProvider& provider) {
        const auto& source = provider.GetNewsSource();
        return !source || source == NMusic::DEFAULT_MORNING_SHOW_NEWS_SOURCE;
    }

    static bool IsDefaultRubric(const NData::TNewsProvider& provider) {
        const auto& rubric = provider.GetRubric();
        return !rubric || rubric == NMusic::DEFAULT_MORNING_SHOW_NEWS_RUBRIC;
    }

    TStringBuf GetPlaylist() const {
        const bool useEveningPlaylist = IsEveningShow() && !Request.HasExpFlag(EXP_HW_EVENING_SHOW_FORCE_PLAYLIST_OF_THE_DAY);
        return useEveningPlaylist ? PLAYLIST_OF_THE_EVENING : PLAYLIST_OF_THE_DAY;
    }

    void ModifierNextTrack(TActionPart& part) {
        const size_t index = Stage.GetTrackIndex();
        Stage.SetTrackIndex(index + 1);

        auto& frame = *part.MutableSemanticFrame()->MutableMusicPlaySemanticFrame();
        frame.MutableTrackOffsetIndex()->SetNumValue(index);
        frame.MutableSpecialPlaylist()->SetStringValue(TString{GetPlaylist()});
    }

    TStringBuf GetRandomRubric() const {
        constexpr std::array mix{
            "business", "computers", "culture", "incident", "index",
            "science", "showbusiness", "society", "sport", "world",
        };
        return Choice(mix, Rng);
    }

    /// \brief Depending on the index, set the rubric and/or location
    void MixNews(TNewsSemanticFrame& frame) const {
        auto& provider = *frame.MutableProvider()->MutableNewsProviderValue();
        if (IsDefaultNewsSource(provider) && IsDefaultRubric(provider)) {
            // mix various news
            if (Stage.GetNewsIndex() == 0 || !TagEvaluator.CheckTag("news_whitelist")) {
                provider.ClearRubric();
            } else {
                provider.SetRubric(TString{GetRandomRubric()});
            }
        }
    }

    // TODO(lavv17): remove when provider will be supported (DIALOG-7656)
    static void SetNewsLegacyTopic(TNewsSemanticFrame& frame) {
        const auto& provider = frame.MutableProvider()->GetNewsProviderValue();
        const TString& topic = IsDefaultNewsSource(provider) ? provider.GetRubric() : provider.GetNewsSource();
        if (topic) {
            frame.MutableTopic()->SetNewsTopicValue(topic);
        }
    }

    bool IsLongNewsDetected() const {
        // TODO(lavv17): improve detection by news duration or some other trait
        return !IsDefaultNewsSource(State.GetSelectedNewsProvider());
    }

    bool PreRenderNews() const {
        if (Stage.GetNewsIndex() && IsLongNewsDetected()) {
            return false;
        }
        return true;
    }

    void ModifierNewsPart(TActionPart& part) {
        AddThinPlayerStopDirectives();
        auto& newsFrame = *part.MutableSemanticFrame()->MutableNewsSemanticFrame();
        if (State.HasSelectedNewsProvider()) {
            newsFrame.MutableNewsIdx()->SetNumValue(Stage.GetNewsIndex());
        } else {
            *State.MutableSelectedNewsProvider() = ChooseNewsProvider();
        }
        *newsFrame.MutableProvider()->MutableNewsProviderValue() = State.GetSelectedNewsProvider();
        MixNews(newsFrame);
        SetNewsLegacyTopic(newsFrame);
        Stage.SetNewsIndex(Stage.GetNewsIndex() + 1);
    }

    void ModifierNewsPhrase(TActionPart& part) {
        const auto& newsFrame = part.GetSemanticFrame().GetNewsSemanticFrame();
        const auto& provider = newsFrame.GetProvider().GetNewsProviderValue();
        if (auto newsPhrase = GetNewsPhrase(provider, part.GetPhraseId()); newsPhrase.Defined()) {
            part.SetPhraseId(std::move(newsPhrase.GetRef()));
        }
    }

    TString GetHardcodedShowType() const {
        TString showType;
        if (GetForChildren()) {
            showType = HARDCODED_SHOW_TYPE_CHILDREN;
        } else if (IsEveningShow()) {
            showType = HARDCODED_SHOW_TYPE_EVENING;
        }
        return showType;
    }

    void ModifierSetHardcodedShowSlots(TActionPart& part) {
        auto& frame = *part.MutableSemanticFrame()->MutableHardcodedMorningShowSemanticFrame();
        frame.MutableOffset()->SetNumValue(Stage.GetHardcodedShowIndex());
        frame.MutableNextTrackIndex()->SetNumValue(Stage.GetTrackIndex());
        if (TString showType = GetHardcodedShowType()) {
            frame.MutableShowType()->SetStringValue(std::move(showType));
        }
        const auto& activateFrame = State.GetActivateFrame().GetAliceShowActivateSemanticFrame();
        if (activateFrame.HasTopic()) {
            frame.MutableTopic()->SetSerializedData(ProtoToJsonString(activateFrame.GetTopic().GetTopicValue()));
        }
        if (activateFrame.HasNewsProvider()) {
            frame.MutableNewsProvider()->SetSerializedData(ProtoToJsonString(activateFrame.GetNewsProvider().GetNewsProviderValue()));
        }
    }

    TString GetRandomNightPlaylistId() const {
        constexpr TStringBuf OWNER_PREFIX = "1335328810:";
        constexpr std::array KIND_RANGE = {
            std::pair{1021, 1037}, // for adults
            std::pair{1001, 1017}, // for children
        };
        const auto& range = KIND_RANGE[GetForChildren()];
        return TString::Join(OWNER_PREFIX, ToString(Rng.RandomInteger(range.first, range.second + 1)));
    }

    void ModifierSetNightPlaylistSlots(TActionPart& part) {
        auto& frame = *part.MutableSemanticFrame()->MutableMusicPlaySemanticFrame();
        frame.MutableObjectId()->SetStringValue(GetRandomNightPlaylistId());
    }

    /// \returns reject phrase name or empty string if not rejected
    TStringBuf GetRejectPhrase() const {
        const auto dayPart = State.GetDayPart().GetValue();
        const auto age = State.GetAge().GetValue();

        if (!Request.ClientInfo().IsSmartSpeaker()) {
            return RENDER_NOT_SUPPORTED;
        }
        if (!NMusic::HasMusicSubscription(Request) && Request.HasExpFlag("hw_disable_alice_show_without_music")) {
            return RENDER_SUBSCRIPTION_REQUIRED;
        }
        if (IsShowTypeAmbigious(dayPart, age)) {
            return RENDER_AMBIGUOUS_SHOW_TYPE;
        }
        return {};
    }

    bool RenderPartByIndex(const size_t index) {
        auto& plan = *State.MutablePlan();
        auto& planHistory = *State.MutablePlanHistory();
        size_t planBaseIndex = 0;
        for (const auto& h : planHistory) {
            planBaseIndex += h.GetActionQueue().ActionsSize();
        }
        while (index < planBaseIndex) {
            plan = std::move(planHistory[planHistory.size() - 1]);
            planHistory.RemoveLast();
            LOG_INFO(Logger) << "Pulled back plan from history: " << plan;
            planBaseIndex -= plan.GetActionQueue().ActionsSize();
        }
        for (size_t loopLimit = 100; loopLimit; --loopLimit) {
            const auto& seq = plan.GetActionQueue().GetActions();
            const int seqIndex = index - planBaseIndex;
            if (seqIndex < seq.size()) {
                LOG_INFO(Logger) << "Show sequence is: " << JoinSeq(" ", seq) << ", current index is: " << seqIndex;
                Stage.SetShowIndex(index + 1);
                return RenderAction(seq[seqIndex]);
            }
            planBaseIndex += seq.size();
            const auto& nextVertex = ChooseNextVertex();
            if (!nextVertex) {
                LOG_INFO(Logger) << "No next vertex found";
                SetIrrelevant();
                return false;
            }
            LOG_INFO(Logger) << "The next vertex is: " << nextVertex;
            FillPlan(nextVertex);
        }
        LOG_ERROR(Logger) << "Infinite loop in config detected";
        SetIrrelevant();
        return false;
    }

    void SetRngSeed(const ui64 seed) {
        Stage.SetRngSeed(seed);
        Rng = TRng{seed};
        LOG_INFO(Logger) << "Using RNG seed " << seed;
    }

    template <typename T>
    static bool ResetConfig(TMutableHolder<T>& configHolder) {
        if (configHolder.Get().GetDefault()) {
            return false;
        }
        auto& config = configHolder.Mutable();
        config.Clear();
        config.SetDefault(true);
        return true;
    }

    void ShowBegin() {
        if (Request.HasExpFlag(EXP_HW_ALICE_SHOW_NEW_USER)) {
            ScenarioData.Mutable().Clear();
            ResetConfig(TopicsConfig);
            ResetConfig(SkillsConfig);
            if (ResetConfig(NewsConfig)) {
                // TODO(lavv17): rewrite config reset after PASKILLS-7977 is done
                *NewsConfig.Mutable().AddNewsProviders() = DefaultNewsProvider();
            }
        }
        if (Request.HasExpFlag(EXP_HW_MORNING_SHOW_RESET_PUSHES)) {
            ScenarioData.Mutable().SetPushesSent(0);
            ScenarioData.Mutable().SetLastPushTimestamp(0);
        }
        if (!ScenarioData.Get().GetRewrittenAfterNewsRubricRemoval() && Request.HasExpFlag(EXP_ALICE_SHOW_NEWS_NO_RUBRICS)) {
            ScenarioData.Mutable().SetLastSuggestTimestamp(0);
            ScenarioData.Mutable().SetSuggestRejectionStreak(0);
            ScenarioData.Mutable().SetPushesSent(0);
            ScenarioData.Mutable().SetLastPushTimestamp(0);
            ScenarioData.Mutable().SetRewrittenAfterNewsRubricRemoval(true);
        }
        if (!TagEvaluator.CheckTag("news") && !NewsConfig.Get().GetDisabled()) {
            NewsConfig.Mutable().SetDisabled(true);
        }
        if (Stage.GetShowIndex()) {
            Stage.Clear();
            State.ClearPlan();
            SetRngSeed(CtxRng.RandomInteger());
        }
        State.ClearStageHistory();
        State.ClearPlanHistory();
        AddStopDirectives();
        RenderPartByIndex(0);
    }

    void AddStageHistory() {
        State.AddStageHistory()->CopyFrom(Stage);
        SetRngSeed(Rng.RandomInteger());
    }

    void PopStageHistory() {
        auto& stageHistory = *State.MutableStageHistory();
        const auto stageHistorySize = stageHistory.size();
        if (stageHistorySize > 0) {
            SetRngSeed(stageHistory[stageHistorySize - 1].GetRngSeed());
            stageHistory.RemoveLast();
        }
    }

    void ShowContinue(const TCallbackDirective* callback) {
        AddStageHistory();
        size_t index = Stage.GetShowIndex();
        if (callback && callback->GetName() == CONTINUE_PART_CALLBACK && index > 0) {
            --index;
        } else {
            Stage.SetShowPartIndex(0);
        }
        if (!RenderPartByIndex(index)) {
            PopStageHistory();
        }
    }

    void ShowNavigateForward() {
        const size_t index = Stage.GetShowIndex();
        if (!index) {
            SetIrrelevant();
            return;
        }
        AddStageHistory();
        if (!RenderPartByIndex(Stage.GetShowPartIndex() ? index - 1 : index)) {
            PopStageHistory();
        }
    }

    void ShowNavigateBackward() {
        auto& stageHistory = *State.MutableStageHistory();
        const auto stageHistorySize = stageHistory.size();
        if (stageHistorySize < 1) {
            SetIrrelevant();
            return;
        }
        if (stageHistorySize >= 2) {
            Stage.CopyFrom(stageHistory[stageHistorySize - 2]);
        } else {
            Stage.Clear();
        }
        PopStageHistory();

        size_t index = Stage.GetShowIndex();
        if (Stage.GetShowPartIndex() && index > 0) {
            --index;
        } else {
            Stage.SetShowPartIndex(0);
        }
        if (!RenderPartByIndex(index)) {
            // reverse callback direction to navigate back again
            if (const auto callback = FindResetAddCallback()) {
                callback->SetName(TString{GET_PREV_SHOW_BLOCK_CALLBACK});
            }
        }
    }

    bool MayNavigate() const {
        const auto start = State.GetStartTimestamp();
        if (!start) {
            LOG_INFO(Logger) << "Show navigation is not allowed because show is not started";
            return false;
        }
        const auto now = GetEpoch();
        const auto secondsSinceStart = now > start ? now - start : 0;
        if (secondsSinceStart >= SECONDS_SINCE_START_THRESHOLD) {
            LOG_INFO(Logger) << "Show navigation is not allowed because show last played at " << start << ", " << secondsSinceStart << " seconds ago";
            return false;
        }
        return true;
    }

    void CheckNavigate() {
        if (!MayNavigate()) {
            SetIrrelevant();
        }
    }

    void AddUserConfigs(TMementoChangeUserObjectsDirective& mementoDirective, EConfigKey key, const NProtoBuf::Message& value) const {
        NMemento::TConfigKeyAnyPair mementoConfig;
        mementoConfig.SetKey(key);
        if (mementoConfig.MutableValue()->PackFrom(value)) {
            *mementoDirective.MutableUserObjects()->AddUserConfigs() = std::move(mementoConfig);
        } else {
            LOG_ERROR(Logger) << "PackFrom failed for user config";
        }
    }

    void UpdateMemento() {
        TMementoChangeUserObjectsDirective mementoDirective;
        if (ScenarioData.Mutated()) {
            NProtoBuf::Any scenarioDataAny;
            if (scenarioDataAny.PackFrom(ScenarioData)) {
                *mementoDirective.MutableUserObjects()->MutableScenarioData() = std::move(scenarioDataAny);
            } else {
                LOG_ERROR(Logger) << "PackFrom failed for scenario data";
            }
        }
        if (NewsConfig.Mutated()) {
            AddUserConfigs(mementoDirective, EConfigKey::CK_MORNING_SHOW_NEWS, NewsConfig);
        }
        if (TopicsConfig.Mutated()) {
            AddUserConfigs(mementoDirective, EConfigKey::CK_MORNING_SHOW_TOPICS, TopicsConfig);
        }
        if (SkillsConfig.Mutated()) {
            AddUserConfigs(mementoDirective, EConfigKey::CK_MORNING_SHOW_SKILLS, SkillsConfig);
        }
        if (mementoDirective.HasUserObjects()) {
            *BodyBuilder.GetResponseBody().AddServerDirectives()->MutableMementoChangeUserObjectsDirective() = std::move(mementoDirective);
        }
    }

    static TString ToSnakeCase(TString s) {
        NProtobufJson::ToSnakeCase(&s);
        return s;
    }

    TString GetAnalyticsShowType() const {
        TString showType = ToSnakeCase(TAliceShowState::EShowType_Name(State.GetShowType()));
        return GetForChildren() && State.GetShowType() != TAliceShowState::Children ? "children_" + showType : std::move(showType);
    }

    TString GetAnalyticsShowAge() const {
        return ToSnakeCase(TAge::EValue_Name(State.GetAge().GetValue()));
    }

    TString GetAnalyticsShowDayPart() const {
        return ToSnakeCase(TDayPart::EValue_Name(State.GetDayPart().GetValue()));
    }

    void Run() {
        auto& analyticsBuilder = BodyBuilder.CreateAnalyticsInfoBuilder();
        analyticsBuilder.SetProductScenarioName(NProductScenarios::ALICE_SHOW);

        if (const auto renderReject = GetRejectPhrase()) {
            RenderPhrase(renderReject);
            if (!FindSemanticFrame(ALICE_SHOW_ACTIVATE_FRAME)) {
                SetIrrelevant();
            }
            analyticsBuilder.AddAction(TString::Join("alice_show.reject.", renderReject),
                                       TString::Join("alice show reject because ", renderReject),
                                       TString::Join("Отказ по причине ", renderReject));
            return;
        }

        if (!Request.Interfaces().GetHasDirectiveSequencer()) {
            // The directive sequiencer is required, but the rejects above can go without it.
            LOG_INFO(Logger) << "directive_sequencer is required, returning Irrelevant";
            SetIrrelevant();
        }

        if (IsNewSessionFrame() || (Request.RequestSource() != TScenarioBaseRequest::GetNext && !Request.IsStackOwner())) {
            BodyBuilder.AddNewSessionStackAction();
        }

        if (FindSemanticFrame(PLAYER_PREV_FRAME) || HasCallback(GET_PREV_SHOW_BLOCK_CALLBACK)) {
            analyticsBuilder.AddAction("alice_show.prev", "alice show previous part", "Перематываем на предыдущую часть шоу Алисы");
            ShowNavigateBackward();
            CheckNavigate();
        } else if (FindSemanticFrame(PLAYER_NEXT_FRAME) || FindSemanticFrame(PLAYER_CONTINUE_FRAME)) {
            analyticsBuilder.AddAction("alice_show.next", "alice show next part", "Перематываем на следующую часть шоу Алисы");
            ShowNavigateForward();
            CheckNavigate();
        } else if (FindSemanticFrame(ACCEPT_SUGGESTED_NEWS_FRAME)) {
            const auto& newsProvider = State.GetNewsSuggest().GetProvider();
            analyticsBuilder.AddAction(
                "alice_show.accept_suggested_news",
                TString::Join("accept suggested news from ", newsProvider.GetNewsSource(), "__", newsProvider.GetRubric()),
                "Добавляем предложенные новости в шоу Алисы"
            );
            ShowNavigateForward();
        } else if (FindSemanticFrame(REJECT_SUGGESTED_NEWS_FRAME)) {
            const auto& newsProvider = State.GetNewsSuggest().GetProvider();
            analyticsBuilder.AddAction(
                "alice_show.reject_suggested_news",
                TString::Join("reject suggested news from ", newsProvider.GetNewsSource(), "__", newsProvider.GetRubric()),
                "Не добавляем предложенные новости в шоу Алисы"
            );
            ShowNavigateForward();
        } else if (const auto callback = Request.Input().GetCallback()) {
            analyticsBuilder.AddAction("alice_show.continue", "alice show goes on", "Шоу Алисы продолжается");
            ShowContinue(callback);
        } else {
            analyticsBuilder.AddAction("alice_show.start", "alice show begins", "Включается шоу Алисы");
            ShowBegin();
        }
        UpdateMemento();

        auto addAnalyticsObject = [&](TStringBuf name, const TString& value) {
            analyticsBuilder.AddObject(TString{name}, value, value);
        };
        addAnalyticsObject("show.type", GetAnalyticsShowType());
        addAnalyticsObject("show.age", GetAnalyticsShowAge());
        addAnalyticsObject("show.day_part", GetAnalyticsShowDayPart());
    }

    static NAnalytics::TAliceShowProfile& AddAliceShowProfile(TScenarioRunResponse& response) {
        auto* property = response.MutableUserInfo()->AddProperties();
        property->SetName("custom-alice-show");
        property->SetId("custom-alice-show");
        property->SetHumanReadable("Alice show customisation");
        return *property->MutableAliceShowProfile();
    }

    std::unique_ptr<TScenarioRunResponse> ReleaseResponse() {
        if (Stage.GetShowIndex() != 0) {
            const auto start = State.GetStartTimestamp();
            const auto now = GetEpoch();
            if (start) {
                const auto secondsSinceStart = now > start ? now - start : 0;
                Builder.FillPlayerFeatures(true, secondsSinceStart);
            }

            // save timestamp for player features of the next part's run
            State.SetStartTimestamp(now);

            // save state only if we started the show
            BodyBuilder.SetState(State);
        }
        std::unique_ptr<TScenarioRunResponse> response = std::move(Builder).BuildResponse();

        if (!NMusic::IsDefaultMorningShowProfile(NewsConfig, TopicsConfig, SkillsConfig)) {
            auto& profile = AddAliceShowProfile(*response);
            *profile.MutableTopicsConfig() = std::move(TopicsConfig.Mutable());
            *profile.MutableNewsConfig() = std::move(NewsConfig.Mutable());
            *profile.MutableSkillsConfig() = std::move(SkillsConfig.Mutable());
        }

        return response;
    }
};

const THashMap<TStringBuf, TAliceShow::TActionPartModifier> TAliceShow::ActionPartModifierMethods = {
    {"news_part", {&TAliceShow::ModifierNewsPart, TTypedSemanticFrame::kNewsSemanticFrame}},
    {"news_phrase", {&TAliceShow::ModifierNewsPhrase, TTypedSemanticFrame::kNewsSemanticFrame}},
    {"next_track", {&TAliceShow::ModifierNextTrack, TTypedSemanticFrame::kMusicPlaySemanticFrame}},
    {"set_hardcoded_show_slots", {&TAliceShow::ModifierSetHardcodedShowSlots, TTypedSemanticFrame::kHardcodedMorningShowSemanticFrame}},
    {"set_night_playlist_slots", {&TAliceShow::ModifierSetNightPlaylistSlots, TTypedSemanticFrame::kMusicPlaySemanticFrame}},
    {"suggest_news_feedback", {&TAliceShow::ModifierSuggestNewsFeedback, {}}},
    {"suggest_news", {&TAliceShow::ModifierSuggestNews, {}}},
};
const THashMap<TStringBuf, TAliceShow::TPreRenderActionMethod> TAliceShow::PreRenderActions = {
    {"choose_news", &TAliceShow::PreRenderChooseNews},
    {"news", &TAliceShow::PreRenderNews},
};
const THashMap<TStringBuf, TAliceShow::TPostRenderActionMethod> TAliceShow::PostRenderActions = {
    {"greeting", &TAliceShow::PostRenderGreeting},
};

} // namespace

void TAliceShowRunHandle::Do(TScenarioHandleContext& ctx) const {
    ctx.ServiceCtx.AddProtobufItem(*TAliceShow{ctx}.MakeResponse(), RESPONSE_ITEM);
}

REGISTER_SCENARIO("alice_show",
                  AddHandle<TAliceShowRunHandle>()
                  .AddFastData<TAliceShowFastDataProto, TAliceShowFastData>("alice_show/alice_show.pb")
                  .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NAliceShow::NNlg::RegisterAll));

} // namespace NAlice::NHollywood
