#include "external_skill.h"
#include "directives.h"

#include <alice/bass/forms/external_skill/skill.h>
#include <alice/bass/forms/search/search.h>

#include <alice/bass/libs/config/config.h>
#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/metrics/metrics.h>
#include <alice/bass/libs/client/experimental_flags.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <alice/library/analytics/common/product_scenarios.h>

#include <library/cpp/protobuf/json/config.h>
#include <library/cpp/protobuf/json/proto2json.h>
#include <library/cpp/resource/resource.h>

#include <util/string/split.h>
#include <util/charset/wide.h>
#include <util/string/cast.h>

namespace NBASS {
namespace {

using namespace NExternalSkill;

constexpr TStringBuf CHANNEL_CHATS = "organizationChat";
constexpr TStringBuf CHANNEL_SKILLS = "aliceSkill";

constexpr TStringBuf SKILL_ID_SLOT_NAME = "skill_id";
constexpr TStringBuf SKILL_ID_SLOT_TYPE = "skill";
constexpr TStringBuf UTTERANCE_NORMALIZED_SLOT_NAME = "utterance_normalized";

constexpr TStringBuf PLATFORM_NAVIGATOR = "navigator";
constexpr TStringBuf PLATFORM_QUASAR = "station";
constexpr TStringBuf PLATFORM_WATCH = "watch";
constexpr TStringBuf PLATFORM_AUTO = "auto";

const TUtf16String SPACE = u" ";
const TVector<TStringBuf> ALLOWED_CHAT_ACTIVATION_PHRASES = {TStringBuf("запусти навык"), TStringBuf("запусти чат")};
const size_t MAX_CANDIDATE_WORD_LEN = 10;
const size_t MAX_CANDIDATES_COUNT = 200;

struct TCandidateResult {
    TCandidateResult(TUtf16String phrase, size_t position, size_t wordsCount)
        : Phrase(std::move(phrase))
        , Position(position)
        , WordsCount(wordsCount)
    {
    }

    TUtf16String Phrase;
    TString SkillId;
    TString Channel;
    size_t Position;
    size_t WordsCount;
};

TErrorBlock::TResult DoChatBot(TContext& ctx, const TSkillDescription& skill) {
    // TODO add stat counters
    TContext::TPtr respCtx = ctx.SetResponseForm(PROCESS_EXTERNAL_SKILL_ACTIVATE_ONLY, false);
    Y_ENSURE(respCtx);
    skill.WriteInfo(respCtx.Get());
    skill.WriteUpdateDialogInfo(respCtx.Get());
    respCtx->CreateSlot(SKILL_ID_SLOT_NAME, SKILL_ID_SLOT_TYPE, true, NSc::TValue(skill.Scheme().Id()));
    return TErrorBlock::Ok;
}

bool IsSkillSupported(const TClientInfo& client, const TSkillDescription& skill) {
    if (skill.IsFromConsole() || !skill.Scheme().HasPlatforms()) {
        return true;
    }

    THashSet<TStringBuf> platforms;
    for (const auto platform : skill.Scheme().Platforms()) {
        platforms.insert(platform);
    }
    if (client.IsNavigator() && !platforms.contains(PLATFORM_NAVIGATOR)) {
        return false;
    }
    if (client.IsSmartSpeaker() && !platforms.contains(PLATFORM_QUASAR)) {
        return false;
    }
    if (client.IsElariWatch() && !platforms.contains(PLATFORM_WATCH)) {
        return false;
    }
    if (client.IsYaAuto() && !platforms.contains(PLATFORM_AUTO)) {
        return false;
    }

    return true;
}

TErrorBlock::TResult DeactivateUnsupportedSkill(TContext& ctx, const TSkillDescription& skill) {
    if (TContext::TPtr respCtx = ctx.SetResponseForm(PROCESS_EXTERNAL_SKILL_DEACTIVATE, false)) {
        skill.WriteInfo(respCtx.Get());
        TContext::TSlot* isUnsupported = respCtx->GetOrCreateSlot("unsupported_platform", "boolean");
        isUnsupported->Value = true;
    }
    return TErrorBlock::Ok;
}

void PrintDiagnosticInfo(const TSkillDiagnosticInfo& diagnosticInfo) {
    NProtobufJson::TProto2JsonConfig config;
    config.SetUseJsonName(true);
    TString outJson = NProtobufJson::Proto2Json(diagnosticInfo, config);
    LOG(INFO) << "Skill diagnostic info: " << outJson << Endl;
    LOG_TYPE(INFO, SKILL_DIAGNOSTIC_INFO) << outJson << Endl;
}

NExternalSkill::TErrorBlock::TResult DoImpl(TContext& ctx, const TContext::TSlot& slotSkill) {
    TSkillDescription skillDescription(ctx);
    skillDescription.Init(slotSkill, ctx);
    const TString& skillNameNorm = skillDescription.GetSkillNameForMetrics();
    if (!skillDescription) {
        Y_STATS_INC_COUNTER("bass_skill_api_error");
        return skillDescription.Result();
    }

    if (!skillDescription.Scheme().BotGuid()->empty()) {
        return DoChatBot(ctx, skillDescription);
    }

    if (!IsSkillSupported(ctx.MetaClientInfo(), skillDescription)) {
        return DeactivateUnsupportedSkill(ctx, skillDescription);
    }

    TSession session(ctx);
    std::unique_ptr<ISkillParser> skill;
    TSkillDiagnosticInfo diagnosticInfo;

    if (TErrorBlock::TResult err = skillDescription.RequestSkill(ctx, session, &skill, diagnosticInfo)) {
        Y_STATS_INC_COUNTER("bass_skill_error");
        skillDescription.WriteInfo(&ctx);
        if (skillDescription.IsDeveloperMode()) {
            err->SetDeveloperMode();
        }
        PrintDiagnosticInfo(diagnosticInfo);
        return err;
    }
    if (!skill && ctx.HasInputAction() && (ctx.InputAction()->Name == ACTION_SKILL_PURCHASE_COMPLETE ||
                                           ctx.InputAction()->Name == ACTION_SKILL_ACCOUNT_LINKING_COMPLETE))
    {
        PrintDiagnosticInfo(diagnosticInfo);
        return TErrorBlock::Ok;
    }

    auto cb = [&ctx, &skillDescription, &session](TContext& respCtx) {
        if (respCtx.FormName() != NExternalSkill::PROCESS_EXTERNAL_SKILL_DEACTIVATE &&
            respCtx.FormName() != NExternalSkill::PROCESS_EXTERNAL_SKILL_ACTIVATE_ONLY &&
            (!skillDescription.OpenInNewTab(ctx) || !ctx.Meta().DialogId()->size()))
        {
            respCtx.AddSuggest(TStringBuf("external_skill_deactivate"));
        }

        if (ctx.MetaClientInfo().IsSmartSpeaker() && session.IsNew()) {
            NSc::TValue v;
            v["listening_is_possible"].SetBool(true);
            ctx.AddCommand<TExternalSkillPlayerPauseDirective>(TStringBuf("player_pause"), std::move(v));
        }
    };

    TContext* respCtx = &ctx;
    if (TErrorBlock::TResult err = skill->CreateVinsAnswer(ctx, &respCtx, cb)) {
        Y_STATS_INC_COUNTER("bass_skill_request_error_responseValidation");
        Y_STATS_INC_COUNTER(TStringBuilder() << "bass_skill_" << skillNameNorm << "_request_error_responseValidation");
        Sensors().Inc(skillDescription.SignalLabels(), "request_error_responseValidation");
        SkillSensors(ctx).Inc(skillDescription.SignalLabels(), "request_error_responseValidation");
        diagnosticInfo.SetErrorType("responseValidation");
        // diagnosticInfo.SetErrorDetail(ToString(err->Data));
        diagnosticInfo.SetErrorDetail(""); // FIXME: ivangromov@: serialize error detail to valid json
        PrintDiagnosticInfo(diagnosticInfo);
        return err;
    }
    session.UpdateContext(*respCtx);

    skillDescription.WriteInfo(respCtx);
    if (session.IsNew()) {
        skillDescription.WriteUpdateDialogInfo(respCtx);
    }

    Y_STATS_INC_COUNTER("bass_skill_request_success");
    Y_STATS_INC_COUNTER(TStringBuilder() << "bass_skill_" << skillNameNorm << "_request_success");
    Sensors().Inc(skillDescription.SignalLabels(), "request_success");
    SkillSensors(ctx).Inc(skillDescription.SignalLabels(), "request_success");

    PrintDiagnosticInfo(diagnosticInfo);
    return TErrorBlock::Ok;
}

THashMap<TUtf16String, TCandidateResult> GenerateCandidates(const TVector<TUtf16String>& words) {
    THashMap<TUtf16String, TCandidateResult> candidates;
    for (size_t len = 1; len <= MAX_CANDIDATE_WORD_LEN; len++) {
        for (size_t i = 0; i + len <= words.size(); i++) {
            // leave only first insertion
            auto phrase = JoinStrings(words.begin() + i, words.begin() + i + len, SPACE.data());
            candidates.insert({phrase, {phrase, i + len, len}});
            if (candidates.size() >= MAX_CANDIDATES_COUNT)
                break;
        }
        if (candidates.size() >= MAX_CANDIDATES_COUNT)
            break;
    }

    return candidates;
}

TVector<TCandidateResult> FindInKvSaas(THashMap<TUtf16String, TCandidateResult>& candidates, const TContext& ctx, bool chatsAllowed) {
    Y_STATS_INC_COUNTER_IF(!ctx.IsTestUser(), "skills_activation_kv_saas_request");
    NHttpFetcher::TRequestPtr request = ctx.GetSources().ExternalSkillsKvSaaS().Request();
    for (const auto& cand : candidates) {
        LOG(DEBUG) << cand.first << Endl;
        request->AddCgiParam(TStringBuf("text"), WideToUTF8(cand.first));
    }

    NHttpFetcher::TResponse::TRef response = request->Fetch()->Wait();
    if (Y_UNLIKELY(!response->IsHttpOk())) {
        Y_STATS_INC_COUNTER_IF(!ctx.IsTestUser(), "skills_activation_kv_saas_error");
        LOG(ERR) << "saas error: " << response->GetErrorText() << Endl;
        return {};
    }

    NSc::TValue answer = NSc::TValue::FromJson(response->Data);
    if (Y_UNLIKELY(answer.IsNull())) {
        Y_STATS_INC_COUNTER_IF(!ctx.IsTestUser(), "skills_activation_kv_saas_bad_answer");
        LOG(ERR) << "empty response from KvSaaS" << Endl;
        return {};
    }

    const NSc::TArray& groups = answer["response"]["results"][0]["groups"].GetArray();
    if (groups.empty()) {
        Y_STATS_INC_COUNTER_IF(!ctx.IsTestUser(), "skills_activation_kv_saas_empty");
        LOG(DEBUG) << "nothing found" << Endl;
        return {};
    }

    TVector<TCandidateResult> foundCandidates;
    for (const NSc::TValue& group: groups) {
        const NSc::TValue& doc = group["documents"][0];
        TStringBuf skillId = doc["properties"]["id"].GetString();
        TStringBuf channel = doc["properties"]["channel"].GetString();
        TUtf16String query = UTF8ToWide(doc["url"].GetString());
        LOG(DEBUG) << "Found: " << skillId << " in channel " << channel << " by " << query << Endl;
        if (!chatsAllowed && channel == CHANNEL_CHATS) {
            LOG(DEBUG) << "Skip chat: " << skillId << Endl;
            continue;
        }
        auto it = candidates.find(query);
        if (it.IsEnd()) {
            LOG(ERR) << "Unknown query from KvSaaS: " << query << Endl;
            continue;
        }
        it->second.SkillId = ToString(skillId);
        it->second.Channel = ToString(channel);
        foundCandidates.push_back(it->second);
    }

    return foundCandidates;
}

bool TryDetectSkill(TContext& ctx, TStringBuf utteranceNormalized, TString& skillId, TString& request) {
    LOG(DEBUG) << utteranceNormalized << Endl;

    TVector<TUtf16String> words;
    StringSplitter(UTF8ToWide(utteranceNormalized).data()).SplitByString(SPACE.data()).SkipEmpty().Collect(&words);
    auto candidates = GenerateCandidates(words);
    const auto chatsAllowed = std::any_of(ALLOWED_CHAT_ACTIVATION_PHRASES.begin(), ALLOWED_CHAT_ACTIVATION_PHRASES.end(),
            [utteranceNormalized] (const auto& phrase) { return utteranceNormalized.Contains(phrase); });
    auto foundCandidates = FindInKvSaas(candidates, ctx, chatsAllowed);
    if (foundCandidates.empty())
        return false;

    std::sort(foundCandidates.begin(), foundCandidates.end(),
            [] (const auto& lhs, const auto& rhs) {
                if (lhs.WordsCount != rhs.WordsCount)
                    return lhs.WordsCount > rhs.WordsCount;
                if (lhs.Channel != rhs.Channel)
                    return lhs.Channel == CHANNEL_SKILLS;
                return lhs.Position < rhs.Position;
            });
    skillId = foundCandidates[0].SkillId;
    request = WideToUTF8(JoinStrings(words.begin() + foundCandidates[0].Position, words.end(), SPACE.data()));

    return true;
}

} // anon namespace

TExternalSkillHandler::~TExternalSkillHandler() = default;

TResultValue TExternalSkillHandler::Do(TRequestHandler& r) {
    using namespace NExternalSkill;

    TContext& ctx = r.Ctx();
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::DIALOGOVO);

    TContext::TSlot* slotSkill = ctx.GetSlot(SKILL_ID_SLOT_NAME);

    if (ctx.HasInputAction() && (ctx.InputAction()->Name == ACTION_SKILL_PURCHASE_COMPLETE ||
                                 ctx.InputAction()->Name == ACTION_SKILL_ACCOUNT_LINKING_COMPLETE))
    {
        if (IsSlotEmpty(slotSkill)) {
            slotSkill = ctx.CreateSlot(SKILL_ID_SLOT_NAME, SKILL_ID_SLOT_TYPE, true /* optional */, ctx.InputAction()->Data[SKILL_ID_SLOT_NAME]);
            LOG(INFO) << slotSkill->ToJson() << Endl;
        }
    }

    if (PROCESS_EXTERNAL_SKILL_DEACTIVATE == ctx.FormName()) {
        if (ctx.HasExpFlag(EXPERIMENTAL_FLAG_CLOSE_EXTERNAL_SKILL_ON_DEACTIVATE)) {
            const auto& meta = ctx.Meta();
            if (meta.DialogId()->size()) {
                NSc::TValue payload;
                payload["dialog_id"] = meta.DialogId();
                ctx.AddCommand<TExternalSkillEndDialogSessionDirective>(TStringBuf("end_dialog_session"), payload);
                if (ctx.ClientFeatures().SupportsExternalSkillAutoClosing()) {
                    ctx.AddCommand<TExternalSkillCloseDialogDirective>(TStringBuf("close_dialog"), payload);
                }
            }
        }
        return ResultSuccess();
    }

    TContext::TSlot* slotNormalized = ctx.GetSlot(UTTERANCE_NORMALIZED_SLOT_NAME);

    if (IsSlotEmpty(slotSkill) && IsSlotEmpty(slotNormalized)) {
        return TError(TError::EType::INVALIDPARAM, "skill_id or utterance_normalized slot is mandatory");
    }

    if (IsSlotEmpty(slotSkill)) {
        const auto utteranceNormalized = slotNormalized->Value.GetString();
        TString skillId;
        TString request;
        if (!TryDetectSkill(ctx, utteranceNormalized, skillId, request)) {
            if (ctx.HasExpFlag("activation_search_redirect_experiment") && TSearchFormHandler::SetAsExternalSkillActivationResponse(ctx, false, ctx.Meta().Utterance()))
                return ctx.RunResponseFormHandler();

            TErrorBlock error(TError::EType::SKILLUNKNOWN, "external skill unknown activation phrase");
            error.InsertIntoContex(&ctx);
            return ResultSuccess();
        }

        slotSkill->Value.SetString(skillId);
        TContext::TSlot* slotRequest = ctx.GetSlot("request");
        slotRequest->Value.SetString(request);
    }

    // all the errors from DoImpl() means block with error and not handler error
    if (const TErrorBlock::TResult err = DoImpl(ctx, *slotSkill)) {
        if (TError::EType::SKILLSERROR == err->Error.Type || TError::EType::SKILLDISABLED == err->Error.Type) {
            err->InsertIntoContex(&ctx);
        }
        else {
            return err->Error;
        }
    }

    return ResultSuccess();
}

void TExternalSkillHandler::RegisterForm(THandlersMap* handlers, IGlobalContext& globalCtx) {
    NExternalSkill::TSkillDescription::InitStylesFromConfig(globalCtx.Config());

    globalCtx.Counters().BassCounters().RegisterUnistatHistogram("extskill_internal_response_time");
    globalCtx.Counters().BassCounters().RegisterUnistatHistogram("extskill_external_response_time");

    globalCtx.Counters().BassCounters().RegisterUnistatHistogram("extskill_yandex_response_time");
    globalCtx.Counters().BassCounters().RegisterUnistatHistogram("extskill_justAI_response_time");
    globalCtx.Counters().BassCounters().RegisterUnistatHistogram("extskill_nonmonitored_response_time");

    for (const auto& t : INTERNAL_SKILLS) {
        globalCtx.Counters().BassCounters().RegisterUnistatHistogram("extskill_" + t.second + "_response_time");
    }

    auto handler = []() {
        return MakeHolder<TExternalSkillHandler>();
    };

    handlers->emplace(NExternalSkill::PROCESS_EXTERNAL_SKILL, handler);
    handlers->emplace(NExternalSkill::PROCESS_EXTERNAL_SKILL_CONTINUE, handler);
    handlers->emplace(NExternalSkill::PROCESS_EXTERNAL_SKILL_DEACTIVATE, handler);
    handlers->emplace(NExternalSkill::PROCESS_EXTERNAL_SKILL_ACTIVATE_ONLY, handler);
}


void TExternalSkillHandler::RegisterAction(THandlersMap* handlers) {
    auto handler = []() {
        return MakeHolder<TExternalSkillHandler>();
    };

    handlers->RegisterActionHandler(NExternalSkill::ACTION_SKILL_PURCHASE_COMPLETE, handler);
    handlers->RegisterActionHandler(NExternalSkill::ACTION_SKILL_ACCOUNT_LINKING_COMPLETE, handler);
}

} // namespace NBASS
