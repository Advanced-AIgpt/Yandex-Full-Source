#include "scene_creators.h"

#include <alice/library/datetime/datetime.h>
#include <alice/library/url_builder/url_builder.h>

#include <util/generic/guid.h>
#include <util/string/join.h>

namespace {

using TAnswerRecord = NAlice::NHollywood::NTimeCapsule::NMemento::TTimeCapsuleInfo::TTimeCapsuleData::TAnswerRecord;
using EQuestionType = TAnswerRecord::EQuestionType;

const TVector<EQuestionType> QUESTIONS_LIST = {
    TAnswerRecord::WhatsUName,
    TAnswerRecord::WhatsUp,
    TAnswerRecord::WhatRUDoingToday,
    TAnswerRecord::RULoveSomeOneNow,
    TAnswerRecord::WhatSGoodHasHappenedRecentlySecond,
    TAnswerRecord::WhoDoUWantThank,
    TAnswerRecord::WhatRUDreamingAboutSecond,
    TAnswerRecord::HaveUBehavedWellThisYear,
    TAnswerRecord::WhatWillURemember2021,
    TAnswerRecord::WhatGiftFromDeadMoroz,
    TAnswerRecord::DoUWantWishUselfSmthForFuture,
    TAnswerRecord::SaySomethingElse
};

constexpr int START_APPROVE_STEP_COUNT = 2;
constexpr int SAVE_APPROVE_STEP_COUNT = 2;
constexpr int MAX_RETRY_COUNT = 1;

constexpr int MAX_SILENCE_DURATION_MS = 2500;
constexpr double EOU_THRESHOLD = 2.0;
constexpr int INITIAL_MAX_SILENCE_DURATION_MS = 10000;
constexpr int STARTING_SILENCE_TIMEOUT_MS = 10000;

constexpr TStringBuf INTERRUPT_KEY = "interrupt";
constexpr TStringBuf START_APPROVE_STEP_KEY = "start_approve_step";
constexpr TStringBuf SAVE_APPROVE_STEP_KEY = "save_approve_step";
constexpr TStringBuf STOP_KEY = "stop";
constexpr TStringBuf QUESTION_ID_KEY = "question_id";
constexpr TStringBuf REQUEST_RETRY_ATTEMPT_ID_KEY = "request_retry_attempt_id";

constexpr TStringBuf S3_ALICE_TIME_CAPSULE_BACKET = "alice-time-capsule";
constexpr TStringBuf DEFAULT_ALICE_AUDIO_FORMAT = ".opus";

constexpr TDuration REQUEST_TIMEOUT = TDuration::Seconds(600);

bool HasSemanticFrame(
    const NAlice::NHollywood::NTimeCapsule::TTimeCapsuleContext& ctx,
    const TStringBuf frameName
) {
    return ctx.RunRequest().Input().FindSemanticFrame(frameName);
}

bool HasApprove(
    const NAlice::NHollywood::NTimeCapsule::TTimeCapsuleContext& ctx
) {
    return HasSemanticFrame(ctx, NAlice::NHollywood::NTimeCapsule::NFrameNames::TIME_CAPSULE_NEXT_STEP) ||
           HasSemanticFrame(ctx, NAlice::NHollywood::NTimeCapsule::NFrameNames::TIME_CAPSULE_START);
}

enum class EActionType {
    HowToDelete,
    NextStep,
    None,
    Resume,
    SkipQuestion,
    Start,
    Stop,
    WhatIsIt,
};

void AddFrameAction(
    const THolder<NAlice::NHollywood::NTimeCapsule::TScene>& scene,
    const TStringBuf frameName,
    EActionType actionType
) {
    NAlice::NScenarios::TFrameAction approveFrameAction;
    approveFrameAction.MutableNluHint()->SetFrameName(frameName.Data());

    switch (actionType) {
    case EActionType::HowToDelete:
        break;
    case EActionType::NextStep:
        approveFrameAction.MutableParsedUtterance()->MutableTypedSemanticFrame()->MutableTimeCapsuleNextStepSemanticFrame();
        break;
    case EActionType::None:
        break;
    case EActionType::Resume:
        approveFrameAction.MutableParsedUtterance()->MutableTypedSemanticFrame()->MutableTimeCapsuleResumeSemanticFrame();
        break;
    case EActionType::SkipQuestion:
        approveFrameAction.MutableParsedUtterance()->MutableTypedSemanticFrame()->MutableTimeCapsuleSkipQuestionSemanticFrame();
        break;
    case EActionType::Start:
        approveFrameAction.MutableParsedUtterance()->MutableTypedSemanticFrame()->MutableTimeCapsuleStartSemanticFrame();
        break;
    case EActionType::Stop:
        approveFrameAction.MutableParsedUtterance()->MutableTypedSemanticFrame()->MutableTimeCapsuleStopSemanticFrame();
        break;
    case EActionType::WhatIsIt:
        break;
    }

    scene->AddAction(frameName.Data(), std::move(approveFrameAction));
}

void AddTimeCapsuleInterruptSuggest(
    const THolder<NAlice::NHollywood::NTimeCapsule::TScene>& resultScene
) {
    resultScene->AddTypeTextSuggest(
        /* type = */ NAlice::NHollywood::NTimeCapsule::NSuggests::TIME_CAPSULE_INTERRUPT.Data(),
        /* suggestsData = */ {},
        /* actionId = */ NAlice::NHollywood::NTimeCapsule::NSuggests::TIME_CAPSULE_INTERRUPT.Data()
    );
}

void AddTimeCapsuleSkipQuestionSuggest(
    const THolder<NAlice::NHollywood::NTimeCapsule::TScene>& resultScene
) {
    resultScene->AddTypeTextSuggest(
        /* type = */ NAlice::NHollywood::NTimeCapsule::NSuggests::TIME_CAPSULE_SKIP_QUESTION.Data(),
        /* suggestsData = */ {},
        /* actionId = */ NAlice::NHollywood::NTimeCapsule::NSuggests::TIME_CAPSULE_SKIP_QUESTION.Data()
    );
}

void AddTimeCapsuleStartSuggest(
    const THolder<NAlice::NHollywood::NTimeCapsule::TScene>& resultScene
) {
    resultScene->AddTypeTextSuggest(
        /* type = */ NAlice::NHollywood::NTimeCapsule::NSuggests::TIME_CAPSULE_START.Data(),
        /* suggestsData = */ {},
        /* actionId = */ NAlice::NHollywood::NTimeCapsule::NSuggests::TIME_CAPSULE_START.Data()
    );
}

void AddTimeCapsuleWhatIsItSuggest(
    const THolder<NAlice::NHollywood::NTimeCapsule::TScene>& resultScene
) {
    resultScene->AddTypeTextSuggest(
        /* type = */ NAlice::NHollywood::NTimeCapsule::NSuggests::TIME_CAPSULE_WHAT_IS_IT.Data(),
        /* suggestsData = */ {},
        /* actionId = */ NAlice::NHollywood::NTimeCapsule::NSuggests::TIME_CAPSULE_WHAT_IS_IT.Data()
    );
}

void SetTimeCapsuleAnalyticsInfo(
    const THolder<NAlice::NHollywood::NTimeCapsule::TScene>& timeCapsuleScene,
    const TString& intentName
) {
    timeCapsuleScene->SetProductScenarioName(NAlice::NHollywood::NTimeCapsule::NScenarioNames::TIME_CAPSULE.Data());
    timeCapsuleScene->SetIntentName(intentName);
}

TString GenerateSessionId(const NAlice::NHollywood::NTimeCapsule::TTimeCapsuleContext& ctx) {
    if (ctx.RunRequest().HasExpFlag(NAlice::NHollywood::NTimeCapsule::NExperiments::HW_TIME_CAPSULE_HARDCODE_SESSION_ID_EXP)) {
        return "00000000-00000000-00000000-00000000";
    }

    return CreateGuidAsString();
}

NAlice::NScenarios::TServerDirective ConstructSaveUserAudioDirective(
    const NAlice::NHollywood::NTimeCapsule::TTimeCapsuleContext& ctx,
    NAlice::NHollywood::TTimeCapsuleState& timeCapsuleState,
    EQuestionType questionType
) {
    NAlice::NScenarios::TServerDirective serverDirective;
    auto* saveUserAudioDirective = serverDirective.MutableSaveUserAudioDirective();

    saveUserAudioDirective->SetPartsToSave(NAlice::NScenarios::TSaveUserAudioDirective::MainPartOnly);

    auto path = JoinSeq(
        "/",
        {
            NAlice::NHollywood::GetUid(ctx.RunRequest()),
            NAlice::NHollywood::NTimeCapsule::NMemento::TTimeCapsuleInfo::TTimeCapsuleData::ETimeCapsuleType_Name(ctx.State().GetTimeCapsuleType()),
            ctx.State().GetSessionInfo().GetSessionId(),
            TStringBuilder{} << ctx.State().GetQuestionStage().GetWaitAnswerForQuestionId() << DEFAULT_ALICE_AUDIO_FORMAT
        }
    );

    auto* s3Storage = saveUserAudioDirective->MutableStorageOptions()->MutableS3Storage();
    s3Storage->SetBucket(S3_ALICE_TIME_CAPSULE_BACKET.Data());
    s3Storage->SetPath(path);

    auto* answerRecord = timeCapsuleState.AddAnswerRecords();
    answerRecord->SetS3Bucket(S3_ALICE_TIME_CAPSULE_BACKET.Data());
    answerRecord->SetS3Path(path);
    answerRecord->SetQuestionType(questionType);

    return serverDirective;
}

NAlice::NScenarios::TServerDirective ConstructUpdateMementoTimeCapsuleUserConfig(
    const NAlice::NHollywood::NTimeCapsule::TTimeCapsuleContext& ctx
) {
    NAlice::NScenarios::TServerDirective serverDirective;
    auto* mementoChangeUserObjectsDirective = serverDirective.MutableMementoChangeUserObjectsDirective();

    NAlice::NHollywood::NTimeCapsule::NMemento::TConfigKeyAnyPair mementoConfig;
    mementoConfig.SetKey(NAlice::NHollywood::NTimeCapsule::NMemento::EConfigKey::CK_TIME_CAPSULE_INFO);

    auto timeCapsuleInfo = ctx.GetMementoTimeCapsuleInfo();

    timeCapsuleInfo.ClearTimeCapsulesData();

    auto* timeCapsuleData = timeCapsuleInfo.AddTimeCapsulesData();

    timeCapsuleData->SetTimeCapsuleId(
        NAlice::NHollywood::NTimeCapsule::NMemento::TTimeCapsuleInfo::TTimeCapsuleData::ETimeCapsuleType_Name(ctx.State().GetTimeCapsuleType())
    );
    timeCapsuleData->SetTimeCapsuleType(ctx.State().GetTimeCapsuleType());

    timeCapsuleData->MutableAnswerRecords()->CopyFrom(ctx.State().GetAnswerRecords());
    timeCapsuleData->SetRecordTime(ctx.RunRequest().ClientInfo().Epoch);
    timeCapsuleData->SetAliceSessionId(ctx.State().GetSessionInfo().GetSessionId());

    if (mementoConfig.MutableValue()->PackFrom(timeCapsuleInfo)) {
        *mementoChangeUserObjectsDirective->MutableUserObjects()->AddUserConfigs() = std::move(mementoConfig);
    }

    return serverDirective;
}

NAlice::NScenarios::TServerDirective ConstructDeleteMementoTimeCapsuleUserConfig(
    const NAlice::NHollywood::NTimeCapsule::TTimeCapsuleContext& ctx
) {
    NAlice::NScenarios::TServerDirective serverDirective;
    auto* mementoChangeUserObjectsDirective = serverDirective.MutableMementoChangeUserObjectsDirective();

    NAlice::NHollywood::NTimeCapsule::NMemento::TConfigKeyAnyPair mementoConfig;
    mementoConfig.SetKey(NAlice::NHollywood::NTimeCapsule::NMemento::EConfigKey::CK_TIME_CAPSULE_INFO);

    auto timeCapsuleInfo = ctx.GetMementoTimeCapsuleInfo();

    timeCapsuleInfo.Clear();

    if (mementoConfig.MutableValue()->PackFrom(timeCapsuleInfo)) {
        *mementoChangeUserObjectsDirective->MutableUserObjects()->AddUserConfigs() = std::move(mementoConfig);
    }

    return serverDirective;
}

void FillNextStateFromLastStepAndClientEpoch(
    const NAlice::NHollywood::NTimeCapsule::TTimeCapsuleContext& ctx,
    NAlice::NHollywood::TTimeCapsuleState& nextState
) {
    nextState.MutableSessionInfo()->SetSessionId(ctx.State().GetSessionInfo().GetSessionId());
    nextState.MutableAnswerRecords()->CopyFrom(ctx.State().GetAnswerRecords());
    nextState.SetTimeCapsuleType(ctx.State().GetTimeCapsuleType());

    nextState.MutableSessionInfo()->SetLastRequestDeviceEpochTime(ctx.RunRequest().ClientInfo().Epoch);
}

bool IsSessionTimedOut(const NAlice::NHollywood::NTimeCapsule::TTimeCapsuleContext& ctx) {
    ui64 lastRequestTimestamp = ctx.State().GetSessionInfo().GetLastRequestDeviceEpochTime();
    ui64 currRequestTimestamp = ctx.RunRequest().ClientInfo().Epoch;

    // Note: TInstant(A) - TInstant(B) = max(0, TInstant(A - B))
    if (TInstant::Seconds(currRequestTimestamp) - TInstant::Seconds(lastRequestTimestamp) > REQUEST_TIMEOUT) {
        return true;
    }

    return false;
}

void ApplyNextTimeCapsuleStateToScene(
    const NAlice::NHollywood::TTimeCapsuleState& nextTimeCapsuleState,
    const THolder<NAlice::NHollywood::NTimeCapsule::TScene>& resultScene,
    const NAlice::NHollywood::NTimeCapsule::TTimeCapsuleContext& ctx
) {
    NJson::TJsonValue cardData;
    TStringBuf nlgTemplate;
    TStringBuf intentName;

    bool isTerminalRequest = false;
    bool needShouldListen = false;

    if (nextTimeCapsuleState.HasStartStage()) {
        if (nextTimeCapsuleState.GetStartStage().GetWaitAnswerForApproveStep() == 1 &&
            !HasSemanticFrame(ctx, NAlice::NHollywood::NTimeCapsule::NFrameNames::TIME_CAPSULE_WHAT_IS_IT)) {
            AddFrameAction(resultScene, NAlice::NHollywood::NTimeCapsule::NFrameNames::TIME_CAPSULE_WHAT_IS_IT, EActionType::WhatIsIt);
            AddTimeCapsuleWhatIsItSuggest(resultScene);
        }

        cardData[START_APPROVE_STEP_KEY] = nextTimeCapsuleState.GetStartStage().GetWaitAnswerForApproveStep();

        nlgTemplate = NAlice::NHollywood::NTimeCapsule::NNlgTemplateNames::TIME_CAPSULE_START_APPROVE;
        intentName = NAlice::NHollywood::NTimeCapsule::NFrameNames::TIME_CAPSULE_START;

        resultScene->AddAnalyticsObject(
            START_APPROVE_STEP_KEY.Data(),
            ToString(nextTimeCapsuleState.GetStartStage().GetWaitAnswerForApproveStep())
        );

        AddFrameAction(resultScene, NAlice::NHollywood::NTimeCapsule::NFrameNames::TIME_CAPSULE_CONFIRM, EActionType::NextStep);
        AddFrameAction(resultScene, NAlice::NHollywood::NTimeCapsule::NFrameNames::TIME_CAPSULE_DECLINE, EActionType::Stop);
        AddFrameAction(resultScene, NAlice::NHollywood::NTimeCapsule::NFrameNames::TIME_CAPSULE_INTERRUPT, EActionType::Stop);

        // temporary confirm form, while alice.time_capsule.confirm empty
        AddFrameAction(resultScene, NAlice::NHollywood::NTimeCapsule::NFrameNames::ALICE_CONFIRM, EActionType::NextStep);
        AddFrameAction(resultScene, NAlice::NHollywood::NTimeCapsule::NFrameNames::ALICE_DECLINE, EActionType::Stop);

        needShouldListen = true;
    } else if (nextTimeCapsuleState.HasQuestionStage()) {
        if (nextTimeCapsuleState.GetQuestionStage().GetWaitApproveToInterrupt()) {
            nlgTemplate = NAlice::NHollywood::NTimeCapsule::NNlgTemplateNames::TIME_CAPSULE_INTERRUPT_APPROVE;
            intentName = NAlice::NHollywood::NTimeCapsule::NFrameNames::TIME_CAPSULE_INTERRUPT;

            AddFrameAction(resultScene, NAlice::NHollywood::NTimeCapsule::NFrameNames::ALICE_CONFIRM, EActionType::Stop);
            AddFrameAction(resultScene, NAlice::NHollywood::NTimeCapsule::NFrameNames::TIME_CAPSULE_INTERRUPT_CONFIRM, EActionType::Stop);
            AddFrameAction(resultScene, NAlice::NHollywood::NTimeCapsule::NFrameNames::ALICE_DECLINE, EActionType::Resume);
            AddFrameAction(resultScene, NAlice::NHollywood::NTimeCapsule::NFrameNames::TIME_CAPSULE_INTERRUPT_DECLINE, EActionType::Resume);
            resultScene->SetShouldListen(true);
        } else {
            AddFrameAction(resultScene, NAlice::NHollywood::NTimeCapsule::NFrameNames::TIME_CAPSULE_FORCE_INTERRUPT, EActionType::None);
            AddFrameAction(resultScene, NAlice::NHollywood::NTimeCapsule::NFrameNames::TIME_CAPSULE_SKIP_QUESTION, EActionType::SkipQuestion);

            AddTimeCapsuleInterruptSuggest(resultScene);
            AddTimeCapsuleSkipQuestionSuggest(resultScene);

            cardData[QUESTION_ID_KEY] = TAnswerRecord::EQuestionType_Name(
                QUESTIONS_LIST[nextTimeCapsuleState.GetQuestionStage().GetWaitAnswerForQuestionId() - 1]
            );

            nlgTemplate = NAlice::NHollywood::NTimeCapsule::NNlgTemplateNames::TIME_CAPSULE_QUESTION;
            intentName = NAlice::NHollywood::NTimeCapsule::NFrameNames::TIME_CAPSULE_QUESTION;

            resultScene->AddAnalyticsObject(
                QUESTION_ID_KEY.Data(),
                ToString(nextTimeCapsuleState.GetQuestionStage().GetWaitAnswerForQuestionId())
            );

            NAlice::NScenarios::TServerDirective serverDirective;
            auto* patchAsrOptionsForNextRequestDirective = serverDirective.MutablePatchAsrOptionsForNextRequestDirective();

            auto* advancedASROptionsPatch = patchAsrOptionsForNextRequestDirective->MutableAdvancedASROptionsPatch();

            advancedASROptionsPatch->MutableMaxSilenceDurationMS()->set_value(MAX_SILENCE_DURATION_MS);
            advancedASROptionsPatch->MutableEnableSidespeechDetector()->set_value(false);
            advancedASROptionsPatch->MutableEouThreshold()->set_value(EOU_THRESHOLD);
            advancedASROptionsPatch->MutableInitialMaxSilenceDurationMS()->set_value(INITIAL_MAX_SILENCE_DURATION_MS);

            resultScene->AddServerDirective(std::move(serverDirective));

            // TODO: do in better way after MEGAMIND-3267
            // ListenDirective is not compatible with ShouldListen on smart speaker
            if (ctx.GetInterfaces().GetHasDirectiveSequencer()) {
                resultScene->AddTtsPlayPlaceholderDirective();

                NAlice::NScenarios::TDirective directive;
                directive.MutableListenDirective()->SetName("listen_directive");
                directive.MutableListenDirective()->SetStartingSilenceTimeoutMs(STARTING_SILENCE_TIMEOUT_MS);

                resultScene->AddDirective(std::move(directive));
            } else {
                needShouldListen = true;
            }

        }
    } else if (nextTimeCapsuleState.HasSaveStage()) {
        if (!HasSemanticFrame(ctx, NAlice::NHollywood::NTimeCapsule::NFrameNames::TIME_CAPSULE_HOW_TO_DELETE)) {
            AddFrameAction(resultScene, NAlice::NHollywood::NTimeCapsule::NFrameNames::TIME_CAPSULE_HOW_TO_DELETE, EActionType::HowToDelete);
        }
        cardData[SAVE_APPROVE_STEP_KEY] = nextTimeCapsuleState.GetSaveStage().GetWaitAnswerForApproveStep();

        nlgTemplate = NAlice::NHollywood::NTimeCapsule::NNlgTemplateNames::TIME_CAPSULE_SAVE_APPROVE;
        intentName = NAlice::NHollywood::NTimeCapsule::NFrameNames::TIME_CAPSULE_SAVE_APPROVE;

        resultScene->AddAnalyticsObject(
            SAVE_APPROVE_STEP_KEY.Data(),
            ToString(nextTimeCapsuleState.GetSaveStage().GetWaitAnswerForApproveStep())
        );

        AddFrameAction(resultScene, NAlice::NHollywood::NTimeCapsule::NFrameNames::TIME_CAPSULE_CONFIRM, EActionType::NextStep);
        AddFrameAction(resultScene, NAlice::NHollywood::NTimeCapsule::NFrameNames::TIME_CAPSULE_DECLINE, EActionType::Stop);
        AddFrameAction(resultScene, NAlice::NHollywood::NTimeCapsule::NFrameNames::TIME_CAPSULE_INTERRUPT, EActionType::Stop);

        // temporary confirm form, while alice.time_capsule.confirm empty
        AddFrameAction(resultScene, NAlice::NHollywood::NTimeCapsule::NFrameNames::ALICE_CONFIRM, EActionType::NextStep);
        AddFrameAction(resultScene, NAlice::NHollywood::NTimeCapsule::NFrameNames::ALICE_DECLINE, EActionType::Stop);

        needShouldListen = true;
    } else if (nextTimeCapsuleState.HasFinishedStage()) {
        nlgTemplate = NAlice::NHollywood::NTimeCapsule::NNlgTemplateNames::TIME_CAPSULE_SAVE;
        intentName = NAlice::NHollywood::NTimeCapsule::NFrameNames::TIME_CAPSULE_SAVE;

        isTerminalRequest = true;
    } else if (nextTimeCapsuleState.HasStopStage()) {
        nlgTemplate = NAlice::NHollywood::NTimeCapsule::NNlgTemplateNames::TIME_CAPSULE_STOP;
        intentName = NAlice::NHollywood::NTimeCapsule::NFrameNames::TIME_CAPSULE_STOP;

        isTerminalRequest = true;
    }

    SetTimeCapsuleAnalyticsInfo(resultScene, intentName.Data());

    if (nextTimeCapsuleState.GetRequestRetryInfo().GetAttemptId() > MAX_RETRY_COUNT) {
        // state must be empty
        resultScene->AddVoiceCard(
            NAlice::NHollywood::NTimeCapsule::NNlgTemplateNames::ERROR,
            NAlice::NHollywood::NTimeCapsule::NNlgTemplateNames::MAX_RETRY_COUNT_REACHED
        );

    } else {
        resultScene->SetState(nextTimeCapsuleState);

        resultScene->AddAnalyticsObject(
            REQUEST_RETRY_ATTEMPT_ID_KEY.Data(),
            ToString(nextTimeCapsuleState.GetRequestRetryInfo().GetAttemptId())
        );

        cardData[REQUEST_RETRY_ATTEMPT_ID_KEY] = nextTimeCapsuleState.GetRequestRetryInfo().GetAttemptId();

        resultScene->AddVoiceCard(
            nlgTemplate,
            NAlice::NHollywood::NTimeCapsule::NNlgTemplateNames::RENDER_RESULT,
            cardData
        );

        if (!isTerminalRequest) {
            resultScene->SetExpectsRequest(true);

            if (needShouldListen) {
                resultScene->SetShouldListen(true);
            }
        }
    }
}

} // namespace

namespace NAlice::NHollywood::NTimeCapsule {

THolder<TScene> TTimeCapsuleStartSceneCreator::TryCreate(const TTimeCapsuleContext& ctx) const {
    TTimeCapsuleState nextTimeCapsuleState;

    if ((ctx.State().GetStageCase() == TTimeCapsuleState::StageCase::STAGE_NOT_SET ||
            ctx.State().HasStopStage() || IsSessionTimedOut(ctx)) &&
        HasSemanticFrame(ctx, NFrameNames::TIME_CAPSULE_START)) {

        nextTimeCapsuleState.MutableStartStage()->SetWaitAnswerForApproveStep(1);
        nextTimeCapsuleState.MutableSessionInfo()->SetSessionId(GenerateSessionId(ctx));
        nextTimeCapsuleState.MutableSessionInfo()->SetLastRequestDeviceEpochTime(ctx.RunRequest().ClientInfo().Epoch);

        if (ctx.RunRequest().HasExpFlag(NExperiments::HW_TIME_CAPSULE_DEMO_MODE_EXP)) {
            nextTimeCapsuleState.SetTimeCapsuleType(NMemento::TTimeCapsuleInfo::TTimeCapsuleData::Demo);
        } else {
            nextTimeCapsuleState.SetTimeCapsuleType(NMemento::TTimeCapsuleInfo::TTimeCapsuleData::NewYear2021);
        }

    } else if (ctx.State().HasStartStage() && (ctx.State().GetStartStage().GetWaitAnswerForApproveStep() < START_APPROVE_STEP_COUNT) &&
                HasApprove(ctx) && !IsSessionTimedOut(ctx)) {

        nextTimeCapsuleState.MutableStartStage()->SetWaitAnswerForApproveStep(
            ctx.State().GetStartStage().GetWaitAnswerForApproveStep() + 1
        );

        FillNextStateFromLastStepAndClientEpoch(ctx, nextTimeCapsuleState);
    } else {
        return nullptr;
    }

    auto resultScene = MakeHolder<TScene>(ctx.Ctx(), ctx.RunRequest());

    if (
        !ctx.RunRequest().ClientInfo().IsSmartSpeaker() &&
        !ctx.RunRequest().ClientInfo().IsSearchApp()
    ) {
        resultScene->AddVoiceCard(
            NAlice::NHollywood::NTimeCapsule::NNlgTemplateNames::ERROR,
            NAlice::NHollywood::NTimeCapsule::NNlgTemplateNames::NOT_SUPPORTED_DEVICE
        );

        SetTimeCapsuleAnalyticsInfo(resultScene, NAlice::NHollywood::NTimeCapsule::NFrameNames::TIME_CAPSULE_START.Data());

        return resultScene;
    }

    if (!NAlice::NHollywood::GetUid(ctx.RunRequest())) {
        const auto authorizationUrl = GenerateAuthorizationUri(
            ctx.GetInterfaces().GetCanOpenYandexAuth()
        );

        if (authorizationUrl) {
            NScenarios::TDirective directive;
            directive.MutableOpenUriDirective()->SetUri(authorizationUrl);

            resultScene->AddDirective(std::move(directive));
        }

        resultScene->AddVoiceCard(
            NNlgTemplateNames::ERROR,
            NNlgTemplateNames::USER_NOT_AUTHORIZED
        );

        return resultScene;
    }

    if (!ctx.RunRequest().HasExpFlag(NExperiments::HW_TIME_CAPSULE_ENABLE_RECORD_EXP)) {
        NJson::TJsonValue cardData;
        cardData["time_capsule_recorded"] = !ctx.GetMementoTimeCapsuleInfo().GetTimeCapsulesData().empty();

        resultScene->AddVoiceCard(
            NNlgTemplateNames::ERROR,
            NNlgTemplateNames::RECORDING_FINISH,
            cardData
        );

        return resultScene;
    }

    if (!ctx.RunRequest().HasExpFlag(NExperiments::HW_TIME_CAPSULE_DEMO_MODE_EXP) &&
        !ctx.GetMementoTimeCapsuleInfo().GetTimeCapsulesData().empty()) {
        resultScene->AddVoiceCard(
            NNlgTemplateNames::ERROR,
            NNlgTemplateNames::TIME_CAPSULE_RECORDED
        );

        return resultScene;
    }

    if (ctx.GetInterfaces().GetCanRenderDiv2Cards()) {
        if (nextTimeCapsuleState.GetStartStage().GetWaitAnswerForApproveStep() == 1) {
            NJson::TJsonValue cardData;
            cardData["init_image_url"] = MakeSharedLinkImageUrl("/get-bass/787408/time_capsule_2", true);

            resultScene->AddDiv2Card(
                NNlgTemplateNames::TIME_CAPSULE_START_APPROVE,
                NNlgTemplateNames::TIME_CAPSULE_IMAGE_CARD,
                cardData
            );
        }
    }

    ApplyNextTimeCapsuleStateToScene(nextTimeCapsuleState, resultScene, ctx);

    return resultScene;
}

THolder<TScene> TTimeCapsuleQuestionSceneCreator::TryCreate(const TTimeCapsuleContext& ctx) const {
    if (IsSessionTimedOut(ctx)) {
        return nullptr;
    }

    TTimeCapsuleState nextTimeCapsuleState;

    if (ctx.State().HasQuestionStage()) {
        if (ctx.State().GetQuestionStage().GetWaitAnswerForQuestionId() < QUESTIONS_LIST.size() &&
            (ctx.RunRequest().Input().Proto().HasVoice() || HasSemanticFrame(ctx, NFrameNames::TIME_CAPSULE_SKIP_QUESTION)) &&
            !HasSemanticFrame(ctx, NFrameNames::TIME_CAPSULE_STOP) &&
            !ctx.State().GetQuestionStage().GetWaitApproveToInterrupt()) {

            nextTimeCapsuleState.MutableQuestionStage()->SetWaitAnswerForQuestionId(
                ctx.State().GetQuestionStage().GetWaitAnswerForQuestionId() + 1
            );

        } else if (ctx.State().GetQuestionStage().GetWaitApproveToInterrupt() && HasSemanticFrame(ctx, NFrameNames::TIME_CAPSULE_RESUME)) {
            nextTimeCapsuleState.MutableQuestionStage()->SetWaitAnswerForQuestionId(
                ctx.State().GetQuestionStage().GetWaitAnswerForQuestionId()
            );

        } else {
            return nullptr;
        }

    } else if (ctx.State().HasStartStage() && (ctx.State().GetStartStage().GetWaitAnswerForApproveStep() == START_APPROVE_STEP_COUNT) && HasApprove(ctx)) {
        nextTimeCapsuleState.MutableQuestionStage()->SetWaitAnswerForQuestionId(1);

    } else {
        return nullptr;
    }

    FillNextStateFromLastStepAndClientEpoch(ctx, nextTimeCapsuleState);

    auto resultScene = MakeHolder<TScene>(ctx.Ctx(), ctx.RunRequest());

    if (ctx.State().HasQuestionStage()  && !ctx.State().GetQuestionStage().GetWaitApproveToInterrupt() &&
        !HasSemanticFrame(ctx, NFrameNames::TIME_CAPSULE_SKIP_QUESTION)) {

        resultScene->AddServerDirective(
            ConstructSaveUserAudioDirective(
                ctx,
                nextTimeCapsuleState,
                QUESTIONS_LIST[ctx.State().GetQuestionStage().GetWaitAnswerForQuestionId() - 1]
            )
        );
    }

    if (HasSemanticFrame(ctx, NFrameNames::TIME_CAPSULE_SKIP_QUESTION)) {
        resultScene->AddAttention(ATTENTION_SKIP_QUESTION);
    }

    ApplyNextTimeCapsuleStateToScene(nextTimeCapsuleState, resultScene, ctx);

    return resultScene;
}

THolder<TScene> TTimeCapsuleSaveApproveSceneCreator::TryCreate(const TTimeCapsuleContext& ctx) const {
    if (IsSessionTimedOut(ctx)) {
        return nullptr;
    }

    TTimeCapsuleState nextTimeCapsuleState;

    if (ctx.State().HasSaveStage() && (ctx.State().GetSaveStage().GetWaitAnswerForApproveStep() < SAVE_APPROVE_STEP_COUNT) && HasApprove(ctx)) {
        nextTimeCapsuleState.MutableSaveStage()->SetWaitAnswerForApproveStep(
            ctx.State().GetSaveStage().GetWaitAnswerForApproveStep() + 1
        );
    } else if (ctx.State().HasQuestionStage() && ctx.State().GetQuestionStage().GetWaitAnswerForQuestionId() == QUESTIONS_LIST.size() &&
        (ctx.RunRequest().Input().Proto().HasVoice() || HasSemanticFrame(ctx, NFrameNames::TIME_CAPSULE_SKIP_QUESTION))) {
        nextTimeCapsuleState.MutableSaveStage()->SetWaitAnswerForApproveStep(1);
    } else {
        return nullptr;
    }

    FillNextStateFromLastStepAndClientEpoch(ctx, nextTimeCapsuleState);

    auto resultScene = MakeHolder<TScene>(ctx.Ctx(), ctx.RunRequest());

    if (ctx.State().HasQuestionStage() && !HasSemanticFrame(ctx, NFrameNames::TIME_CAPSULE_SKIP_QUESTION)) {
        resultScene->AddServerDirective(
            ConstructSaveUserAudioDirective(
                ctx,
                nextTimeCapsuleState,
                QUESTIONS_LIST[ctx.State().GetQuestionStage().GetWaitAnswerForQuestionId() - 1]
            )
        );
    }

    if (HasSemanticFrame(ctx, NFrameNames::TIME_CAPSULE_SKIP_QUESTION)) {
        resultScene->AddAttention(ATTENTION_SKIP_QUESTION);
    }

    ApplyNextTimeCapsuleStateToScene(nextTimeCapsuleState, resultScene, ctx);

    return resultScene;
}

THolder<TScene> TTimeCapsuleStopSceneCreator::TryCreate(const TTimeCapsuleContext& ctx) const {
    if (IsSessionTimedOut(ctx)) {
        return nullptr;
    }

    TTimeCapsuleState nextTimeCapsuleState;

    if (HasSemanticFrame(ctx, NFrameNames::TIME_CAPSULE_STOP) ||
        (ctx.State().GetQuestionStage().GetWaitApproveToInterrupt() && !HasSemanticFrame(ctx, NFrameNames::TIME_CAPSULE_RESUME))) {
        nextTimeCapsuleState.MutableStopStage();
    } else {
        return nullptr;
    }

    auto resultScene = MakeHolder<TScene>(ctx.Ctx(), ctx.RunRequest());

    FillNextStateFromLastStepAndClientEpoch(ctx, nextTimeCapsuleState);

    ApplyNextTimeCapsuleStateToScene(nextTimeCapsuleState, resultScene, ctx);

    resultScene->AddAnalyticsObject(
        STOP_KEY.Data(),
        PrintToString(ctx.State())
    );

    return resultScene;
}

THolder<TScene> TTimeCapsuleInterruptSceneCreator::TryCreate(const TTimeCapsuleContext& ctx) const {
    if (IsSessionTimedOut(ctx)) {
        return nullptr;
    }

    TTimeCapsuleState nextTimeCapsuleState;

    if ((HasSemanticFrame(ctx, NFrameNames::TIME_CAPSULE_INTERRUPT) || HasSemanticFrame(ctx, NFrameNames::TIME_CAPSULE_FORCE_INTERRUPT)) && ctx.State().HasQuestionStage() && !ctx.State().GetQuestionStage().GetWaitApproveToInterrupt()) {
        nextTimeCapsuleState.MutableQuestionStage()
                            ->SetWaitAnswerForQuestionId(ctx.State().GetQuestionStage().GetWaitAnswerForQuestionId());
        nextTimeCapsuleState.MutableQuestionStage()
                            ->SetWaitApproveToInterrupt(true);
    } else {
        return nullptr;
    }

    auto resultScene = MakeHolder<TScene>(ctx.Ctx(), ctx.RunRequest());

    FillNextStateFromLastStepAndClientEpoch(ctx, nextTimeCapsuleState);

    ApplyNextTimeCapsuleStateToScene(nextTimeCapsuleState, resultScene, ctx);

    resultScene->AddAnalyticsObject(
        INTERRUPT_KEY.Data(),
        ToString(nextTimeCapsuleState.GetQuestionStage().GetWaitAnswerForQuestionId())
    );

    return resultScene;
}

THolder<TScene> TTimeCapsuleSaveSceneCreator::TryCreate(const TTimeCapsuleContext& ctx) const {
    if (IsSessionTimedOut(ctx)) {
        return nullptr;
    }

    if (ctx.State().HasSaveStage() && (ctx.State().GetSaveStage().GetWaitAnswerForApproveStep() == SAVE_APPROVE_STEP_COUNT) && HasApprove(ctx)) {
        auto resultScene = MakeHolder<TScene>(ctx.Ctx(), ctx.RunRequest());

        resultScene->AddServerDirective(
            ConstructUpdateMementoTimeCapsuleUserConfig(ctx)
        );

        TTimeCapsuleState nextTimeCapsuleState;
        nextTimeCapsuleState.MutableFinishedStage();

        FillNextStateFromLastStepAndClientEpoch(ctx, nextTimeCapsuleState);

        ApplyNextTimeCapsuleStateToScene(nextTimeCapsuleState, resultScene, ctx);

        return resultScene;
    } else {
        return nullptr;
    }
}

THolder<TScene> TTimeCapsuleApproveRetrySceneCreator::TryCreate(const TTimeCapsuleContext& ctx) const {
    if (IsSessionTimedOut(ctx)) {
        return nullptr;
    }

    auto nextTimeCapsuleState = ctx.State();
    auto resultScene = MakeHolder<TScene>(ctx.Ctx(), ctx.RunRequest());

    if (ctx.State().HasStartStage() && !HasApprove(ctx) && !HasSemanticFrame(ctx, NFrameNames::TIME_CAPSULE_STOP)) {
        if (nextTimeCapsuleState.GetStartStage().GetWaitAnswerForApproveStep() == 1 &&
            HasSemanticFrame(ctx, NAlice::NHollywood::NTimeCapsule::NFrameNames::TIME_CAPSULE_WHAT_IS_IT)) {
            resultScene->AddAttention(NAlice::NHollywood::NTimeCapsule::ATTENTION_WHAT_IS_IT);
        } else {
            nextTimeCapsuleState.MutableRequestRetryInfo()
                                ->SetAttemptId(ctx.State().GetRequestRetryInfo().GetAttemptId() + 1);
        }
    } else if (ctx.State().HasSaveStage() && !HasApprove(ctx) && !HasSemanticFrame(ctx, NFrameNames::TIME_CAPSULE_STOP)) {
        if (HasSemanticFrame(ctx, NAlice::NHollywood::NTimeCapsule::NFrameNames::TIME_CAPSULE_HOW_TO_DELETE)) {
            resultScene->AddAttention(NAlice::NHollywood::NTimeCapsule::ATTENTION_HOW_TO_DELETE);
        } else {
            nextTimeCapsuleState.MutableRequestRetryInfo()
                                ->SetAttemptId(ctx.State().GetRequestRetryInfo().GetAttemptId() + 1);
        }
    } else {
        return nullptr;
    }

    nextTimeCapsuleState.MutableSessionInfo()->SetLastRequestDeviceEpochTime(ctx.RunRequest().ClientInfo().Epoch);

    if (ctx.RunRequest().Input().Proto().HasText()) {
        resultScene->AddAttention(ATTENTION_TEXT_ANSWER);
    }

    ApplyNextTimeCapsuleStateToScene(nextTimeCapsuleState, resultScene, ctx);

    return resultScene;
}

THolder<TScene> TTimeCapsuleTextRetrySceneCreator::TryCreate(const TTimeCapsuleContext& ctx) const {
    if (IsSessionTimedOut(ctx)) {
        return nullptr;
    }

    auto nextTimeCapsuleState = ctx.State();

    if (ctx.State().HasQuestionStage() && ctx.RunRequest().Input().Proto().HasText() && !HasSemanticFrame(ctx, NFrameNames::TIME_CAPSULE_SKIP_QUESTION)) {
        nextTimeCapsuleState.MutableRequestRetryInfo()
                            ->SetAttemptId(ctx.State().GetRequestRetryInfo().GetAttemptId() + 1);
    } else {
        return nullptr;
    }

    nextTimeCapsuleState.MutableSessionInfo()->SetLastRequestDeviceEpochTime(ctx.RunRequest().ClientInfo().Epoch);

    auto resultScene = MakeHolder<TScene>(ctx.Ctx(), ctx.RunRequest());

    resultScene->AddAttention(ATTENTION_TEXT_ANSWER);

    ApplyNextTimeCapsuleStateToScene(nextTimeCapsuleState, resultScene, ctx);

    return resultScene;
}

THolder<TScene> TTimeCapsuleInformationSceneCreator::TryCreate(const TTimeCapsuleContext& ctx) const {
    TStringBuf phraseName;
    TStringBuf frameName;
    if (HasSemanticFrame(ctx, NFrameNames::TIME_CAPSULE_HOW_TO_DELETE)) {
        phraseName = NNlgTemplateNames::HOW_TO_DELETE;
        frameName = NFrameNames::TIME_CAPSULE_HOW_TO_DELETE;
    } else if (HasSemanticFrame(ctx, NFrameNames::TIME_CAPSULE_HOW_TO_OPEN)) {
        phraseName = NNlgTemplateNames::HOW_TO_OPEN;
        frameName = NFrameNames::TIME_CAPSULE_HOW_TO_OPEN;
    } else if (HasSemanticFrame(ctx, NFrameNames::TIME_CAPSULE_HOW_TO_RECORD)) {
        phraseName = NNlgTemplateNames::HOW_TO_RECORD;
        frameName = NFrameNames::TIME_CAPSULE_HOW_TO_RECORD;
    } else if (HasSemanticFrame(ctx, NFrameNames::TIME_CAPSULE_HOW_TO_RERECORD)) {
        phraseName = NNlgTemplateNames::HOW_TO_RERECORD;
        frameName = NFrameNames::TIME_CAPSULE_HOW_TO_RERECORD;
    } else if (HasSemanticFrame(ctx, NFrameNames::TIME_CAPSULE_WHAT_IS_IT)) {
        phraseName = NNlgTemplateNames::WHAT_IS_IT;
        frameName = NFrameNames::TIME_CAPSULE_WHAT_IS_IT;
    } else {
        return nullptr;
    }

    auto resultScene = MakeHolder<TScene>(ctx.Ctx(), ctx.RunRequest());

    SetTimeCapsuleAnalyticsInfo(resultScene, frameName.Data());

    const bool recorded = !ctx.GetMementoTimeCapsuleInfo().GetTimeCapsulesData().empty();

    if (!recorded) {
        AddFrameAction(resultScene, NAlice::NHollywood::NTimeCapsule::NFrameNames::ALICE_CONFIRM, EActionType::Start);
        AddTimeCapsuleStartSuggest(resultScene);

        resultScene->SetShouldListen(true);
    }

    NJson::TJsonValue cardData;
    cardData["time_capsule_recorded"] = recorded;

    resultScene->AddVoiceCard(
        NNlgTemplateNames::TIME_CAPSULE_INFORMATION,
        phraseName,
        cardData
    );

    return resultScene;
}

THolder<TScene> TTimeCapsuleDeleteSceneCreator::TryCreate(const TTimeCapsuleContext& ctx) const {
    if (HasSemanticFrame(ctx, NFrameNames::TIME_CAPSULE_DELETE)) {
        auto resultScene = MakeHolder<TScene>(ctx.Ctx(), ctx.RunRequest());

        SetTimeCapsuleAnalyticsInfo(resultScene, NFrameNames::TIME_CAPSULE_DELETE.Data());

        NJson::TJsonValue cardData;

        if (ctx.RunRequest().HasExpFlag(NExperiments::HW_TIME_CAPSULE_DEMO_MODE_EXP)) {
            resultScene->AddServerDirective(ConstructDeleteMementoTimeCapsuleUserConfig(ctx));
            cardData["time_capsule_deleted"] = true;
        }

        resultScene->AddVoiceCard(
            NNlgTemplateNames::TIME_CAPSULE_DELETE,
            NNlgTemplateNames::RENDER_RESULT,
            cardData
        );

        return resultScene;
    } else {
        return nullptr;
    }
}

THolder<TScene> TTimeCapsuleRerecordSceneCreator::TryCreate(const TTimeCapsuleContext& ctx) const {
    if (HasSemanticFrame(ctx, NFrameNames::TIME_CAPSULE_RERECORD)) {
        auto resultScene = MakeHolder<TScene>(ctx.Ctx(), ctx.RunRequest());

        SetTimeCapsuleAnalyticsInfo(resultScene, NFrameNames::TIME_CAPSULE_RERECORD.Data());

        resultScene->AddVoiceCard(
            NNlgTemplateNames::TIME_CAPSULE_RERECORD,
            NNlgTemplateNames::RENDER_RESULT
        );

        return resultScene;
    } else {
        return nullptr;
    }
}

THolder<TScene> TTimeCapsuleOpenSceneCreator::TryCreate(const TTimeCapsuleContext& ctx) const {
    if (HasSemanticFrame(ctx, NFrameNames::TIME_CAPSULE_OPEN)) {
        auto resultScene = MakeHolder<TScene>(ctx.Ctx(), ctx.RunRequest());

        SetTimeCapsuleAnalyticsInfo(resultScene, NFrameNames::TIME_CAPSULE_OPEN.Data());

        NJson::TJsonValue cardData;
        cardData["time_capsule_recorded"] = !ctx.GetMementoTimeCapsuleInfo().GetTimeCapsulesData().empty();
        cardData["enable_record"] = ctx.RunRequest().HasExpFlag(NExperiments::HW_TIME_CAPSULE_ENABLE_RECORD_EXP);

        resultScene->AddVoiceCard(
            NNlgTemplateNames::TIME_CAPSULE_OPEN,
            NNlgTemplateNames::RENDER_RESULT,
            cardData
        );

        return resultScene;
    } else {
        return nullptr;
    }
}

THolder<TScene> TTimeCapsuleHowLongSceneCreator::TryCreate(const TTimeCapsuleContext& ctx) const {
    if (HasSemanticFrame(ctx, NFrameNames::TIME_CAPSULE_HOW_LONG)) {
        auto resultScene = MakeHolder<TScene>(ctx.Ctx(), ctx.RunRequest());

        SetTimeCapsuleAnalyticsInfo(resultScene, NFrameNames::TIME_CAPSULE_HOW_LONG.Data());

        auto timeCapsuleInfo = ctx.GetMementoTimeCapsuleInfo();

        if (!timeCapsuleInfo.GetTimeCapsulesData().empty()) {
            auto timeCapsuleData = timeCapsuleInfo.GetTimeCapsulesData().Get(0);

            auto remainingTimestamp = TInstant::Seconds(timeCapsuleData.GetRecordTime()) + TDuration::Days(365);


            NJson::TJsonValue cardData;
            cardData["how_long"] = remainingTimestamp.Seconds();

            resultScene->AddVoiceCard(
                NNlgTemplateNames::TIME_CAPSULE_HOW_LONG,
                NNlgTemplateNames::RENDER_RESULT,
                cardData
            );
        } else {
            resultScene->AddVoiceCard(
                NNlgTemplateNames::TIME_CAPSULE_HOW_LONG,
                NNlgTemplateNames::NO_TIME_CAPSULES
            );
        }

        return resultScene;
    } else {
        return nullptr;
    }
}

THolder<TScene> TTimeCapsuleIrrelevantSceneCreator::TryCreate(const TTimeCapsuleContext& ctx) const {
    auto resultScene = MakeHolder<TScene>(ctx.Ctx(), ctx.RunRequest());

    resultScene->AddVoiceCard(
        NNlgTemplateNames::ERROR,
        NNlgTemplateNames::NOT_SUPPORTED
    );

    resultScene->SetIrrelevant();

    return resultScene;
}

} // namespace NAlice::NHollywood::NTimeCapsule
