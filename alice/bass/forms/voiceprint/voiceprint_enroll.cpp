// Code in this file implements logic described here:
// https://wiki.yandex-team.ru/assistant/dialogs/new-voiceprint-enroll/

#include "voiceprint_enroll.h"
#include "voiceprint_utils.h"

#include <alice/bass/forms/common/biometry_delegate.h>
#include <alice/bass/forms/common/uid_utils.h>
#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/directives.h>

#include <alice/bass/libs/client/experimental_flags.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/util/error.h>

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/biometry/biometry.h>

#include <kernel/geodb/countries.h>

#include <util/digest/murmur.h>
#include <util/generic/string.h>
#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/system/types.h>
#include <util/system/yassert.h>

#include <cmath>
#include <cstddef>
#include <utility>

namespace NBASS {

namespace {

constexpr TStringBuf ENROLL = "personal_assistant.scenarios.voiceprint_enroll";
constexpr TStringBuf ENROLL_COLLECT_VOICE = "personal_assistant.scenarios.voiceprint_enroll__collect_voice";
constexpr TStringBuf ENROLL_FINISH = "personal_assistant.scenarios.voiceprint_enroll__finish";
constexpr TStringBuf WHAT_IS_MY_NAME = "personal_assistant.scenarios.what_is_my_name";
constexpr TStringBuf SET_MY_NAME = "personal_assistant.scenarios.set_my_name";

constexpr TStringBuf SLOT_CREATED_UID = "created_uid";
constexpr TStringBuf SLOT_CREATED_USER_SECRET = "created_user_secret";
constexpr TStringBuf SLOT_DISTRACTOR = "distractor";
constexpr TStringBuf SLOT_IS_CHANGE_NAME = "is_change_name";
constexpr TStringBuf SLOT_IS_KNOWN = "is_known";
constexpr TStringBuf SLOT_IS_NEED_EXPLAIN = "is_need_explain";
constexpr TStringBuf SLOT_IS_NEED_LOGIN = "is_need_login";
constexpr TStringBuf SLOT_IS_SERVER_ERROR = "is_server_error";
constexpr TStringBuf SLOT_IS_SERVER_REPEAT = "is_server_repeat";
constexpr TStringBuf SLOT_IS_TOO_MANY_ENROLLED_USERS = "is_too_many_enrolled_users";
constexpr TStringBuf SLOT_IS_USERNAME_ERROR = "is_username_error";
constexpr TStringBuf SLOT_OLD_USER_NAME = "old_user_name";
constexpr TStringBuf SLOT_PHRASES_COUNT = "phrases_count";
constexpr TStringBuf SLOT_READY = "ready";
constexpr TStringBuf SLOT_READY_FROZEN = "ready_frozen";
constexpr TStringBuf SLOT_SERVER_REPEAT_COUNT = "server_repeat_count";
constexpr TStringBuf SLOT_USER_NAME = "user_name";
constexpr TStringBuf SLOT_USER_NAME_FROZEN = "user_name_frozen";
constexpr TStringBuf SLOT_USER_REPEAT = "user_repeat";
constexpr TStringBuf SLOT_USERNAME_REPEAT_COUNT = "username_repeat_count";
constexpr TStringBuf SLOT_VOICE_REQUESTS = "voice_requests";

constexpr TStringBuf ATTENTION_ENROLL_REQUESTED = "what_is_my_name__enroll_requested";
constexpr TStringBuf ATTENTION_KNOWN_USER = "known_user";
constexpr TStringBuf ATTENTION_SILENT_ENROLL_MODE = "what_is_my_name__silent_enroll_mode";
constexpr TStringBuf ATTENTION_INVALID_REGION = "invalid_region";

constexpr TStringBuf TYPE_BOOL = "bool";
constexpr TStringBuf TYPE_INT = "int";
constexpr TStringBuf TYPE_LIST = "list";
constexpr TStringBuf TYPE_STRING = "string";

constexpr TStringBuf ACTION_SAVE_VOICEPRINT = "save_voiceprint";

constexpr TStringBuf INVALID_BIO_CONFIG_MESSAGE = "Biometrics server sent invalid configuration";

constexpr i64 MAX_ENROLLED_USERS = 1;
constexpr i64 SERVER_MAX_REPEAT = 3;
constexpr i64 TOTAL_ENROLL_PHRASES = 5;
constexpr i64 USERNAME_MAX_REPEAT = 2;

using namespace NAlice::NBiometry;

bool IsSlotEmptyString(const TSlot* slot) {
    if (IsSlotEmpty(slot)) {
        return true;
    }
    auto value = slot->Value;
    if (!value.IsString()) {
        return true;
    }
    return value.GetString().empty();
}

void SetIsKnown(TContext& ctx, bool isKnown) {
    ctx.CreateSlot(SLOT_IS_KNOWN /* name */, TYPE_BOOL, true /* optional */, NSc::TValue().SetBool(isKnown));
}

void SetSilentMode(TContext& ctx) {
    ctx.AddAttention(ATTENTION_SILENT_ENROLL_MODE);
}

TContext& GetActualResponseForm(TContext& ctx) {
    TContext* responseForm = ctx.GetResponseForm().Get();
    if (responseForm == nullptr) {
        responseForm = &ctx;
    }
    return *responseForm;
}

TResultValue GetUidAndUsername(TContext& ctx, TMaybe<TString>& uid, TMaybe<TString>& userName,
                               TBlackBoxAPI& blackBoxAPI, TDataSyncAPI& dataSyncAPI) {
    TString retrievedValue;

    auto uidAcquireType = GetUidAcquireType(ctx);

    Y_ASSERT(uidAcquireType != EUidAcquireType::BIOMETRY);

    switch (uidAcquireType) {
        case EUidAcquireType::UNAUTHORIZED:
            retrievedValue = GetUnauthorizedUid(ctx);
            break;
        case EUidAcquireType::BLACK_BOX:
            if (!blackBoxAPI.GetUid(ctx, retrievedValue)) {
                SetError(GetActualResponseForm(ctx), "Error requesting UID from BlackBox API",
                         true /* addErrorBlock */);
                return TError{TError::EType::SYSTEM};
            }
            break;
        case EUidAcquireType::BIOMETRY:
            Y_FAIL("Unsupported uidAcquireType");
            return TResultValue{};
    }

    uid = std::move(retrievedValue);

    const auto error =
        dataSyncAPI.Get(ctx, *uid.Get(), TPersonalDataHelper::EUserSpecificKey::UserName, retrievedValue);

    if (error) {
        if (error->Type != TError::EType::NODATASYNCKEYFOUND) {
            LOG(ERR) << "Cannot get user name from DataSync (uid: " << uid << "): " << error->Msg << Endl;
            return error;
        }
    } else {
        userName = std::move(retrievedValue);
    }
    return ResultSuccess();
}

TResultValue GetBiometryUidAndUsername(NAlice::NBiometry::TBiometry& biometry, TMaybe<TString>& uid, TMaybe<TString>& userName) {
    if (!biometry.HasScores() || !biometry.IsKnownUser()) {
        return TResultValue{};
    }

    TString retrievedValue;

    biometry.GetUserId(retrievedValue);
    uid = std::move(retrievedValue);

    if (const auto error = biometry.GetUserName(retrievedValue)) {
        return TError{TError::EType::BIOMETRY, error->Msg};
    }
    userName = std::move(retrievedValue);
    return TResultValue{};
}

void AddSaveVoiceprintAction(TContext& ctx, TStringBuf uid, const NSc::TValue& voiceRequests) {
    NSc::TValue data;
    data["user_id"] = uid;
    data["requests"] = voiceRequests;
    ctx.AddUniProxyAction(ACTION_SAVE_VOICEPRINT, std::move(data));
}

void SetDataSyncRequestError(TContext& ctx, TResultValue error, bool addErrorBlock) {
    SetError(ctx, "Error requesting DataSyncAPI", *error, addErrorBlock);
}

bool CheckNeedLogin(TContext& ctx) {
    if (!ctx.IsAuthorizedUser()) {
        TContext::TPtr finish = ctx.SetResponseForm(ENROLL_FINISH, false /* setCurrentFormAsCallback */);
        Y_ENSURE(finish);
        auto* const isNeedLogin = finish->GetOrCreateSlot(SLOT_IS_NEED_LOGIN, TYPE_BOOL);
        isNeedLogin->Value.SetBool(true);
        return true;
    }
    return false;
}

bool AreTooManyUsers(const TContext& ctx) {
    if (!ctx.HasExpFlag(EXPERIMENTAL_FLAG_BIO_LIMIT_ENROLLED_USERS) || !HasNonEmptyBiometricsScores(ctx.Meta())) {
        return false;
    }
    auto scoresSize = 0;
    if (ctx.Meta().BiometricsScores().HasScoresWithMode()) {
        scoresSize = ctx.Meta().BiometricsScores().ScoresWithMode()[0].Scores().Size();
    } else {
        scoresSize = ctx.Meta().BiometricsScores().Scores().Size();
    }
    return scoresSize >= MAX_ENROLLED_USERS;
}

bool CheckTooManyUsers(TContext& ctx, TBlackBoxAPI& blackBoxApi, TDataSyncAPI& dataSyncApi) {
    if (!AreTooManyUsers(ctx)) {
        return false;
    }

    TContext::TPtr finish = ctx.SetResponseForm(ENROLL_FINISH, false /* setCurrentFormAsCallback */);
    Y_ENSURE(finish);
    auto* const isTooManyEnrolledUsers = finish->GetOrCreateSlot(SLOT_IS_TOO_MANY_ENROLLED_USERS, TYPE_BOOL);
    isTooManyEnrolledUsers->Value.SetBool(true);

    TBiometryDelegate delegate{ctx, blackBoxApi, dataSyncApi};
    TBiometry biometry(ctx.Meta(), delegate, TBiometry::EMode::HighTPR);
    if (biometry.IsKnownUser()) {
        finish->AddAttention(ATTENTION_KNOWN_USER);
    }
    return true;
}

void ProcessCountableError(TContext& ctx, TStringBuf countSlotName, TStringBuf isErrorSlotName,
                           TMaybe<TStringBuf> isRepeatSlotName, int maxRepeat) {
    auto* countSlot = GetSlotTyped(ctx, countSlotName, NSc::TValue::EType::IntNumber);
    if (!countSlot) {
        countSlot = ctx.CreateSlot(countSlotName, TYPE_INT);
        countSlot->Value.SetIntNumber(0);
    }
    auto repeatCount = countSlot->Value.GetIntNumber();
    if (repeatCount < maxRepeat) {
        if (isRepeatSlotName.Defined()) {
            auto* isRepeatSlot = ctx.GetOrCreateSlot(*isRepeatSlotName, TYPE_BOOL);
            isRepeatSlot->Value.SetBool(true);
        }
        ++repeatCount;
        countSlot->Value.SetIntNumber(repeatCount);
        return;
    }
    TContext::TPtr finish = ctx.SetResponseForm(ENROLL_FINISH, false /* setCurrentFormAsCallback */);
    Y_ENSURE(finish);
    for (const auto blockRef : ctx.GetBlocks()) {
        const auto& block = blockRef.get();
        *finish->Block() = block;
    }
    auto* isErrorSlot = finish->GetOrCreateSlot(isErrorSlotName, TYPE_BOOL);
    isErrorSlot->Value.SetBool(true);
}

void ProcessServerError(TContext& ctx) {
    ProcessCountableError(ctx, SLOT_SERVER_REPEAT_COUNT, SLOT_IS_SERVER_ERROR, SLOT_IS_SERVER_REPEAT,
                          SERVER_MAX_REPEAT);
}

void ProcessUsernameError(TContext& ctx) {
    ProcessCountableError(ctx, SLOT_USERNAME_REPEAT_COUNT, SLOT_IS_USERNAME_ERROR, {}, USERNAME_MAX_REPEAT);
}

void AddListeningPlayerPauseCommand(TContext& ctx) {
    NSc::TValue payload;
    payload["listening_is_possible"].SetBool(true);
    ctx.AddCommand<TVoicePrintPlayerPauseDirective>(TStringBuf("player_pause"), std::move(payload));
}

bool IsSlotSwear(const TSlot* slot) {
    return !IsSlotEmpty(slot) && slot->Type == "swear";
}

bool CheckInvalidRegion(TContext& ctx, bool changeFormToEnrollFinish) {
    const NGeobase::TLookup& geobase = ctx.GlobalCtx().GeobaseLookup();
    NGeobase::TId userRegion = ctx.UserRegion();
    if (!NAlice::IsValidId(userRegion) || geobase.IsIdInRegion(userRegion, NGeoDB::RUSSIA_ID)) {
        return false;
    }
    if (changeFormToEnrollFinish) {
        ctx.SetResponseForm(ENROLL_FINISH, false /* setCurrentFormAsCallback */);
    }
    GetActualResponseForm(ctx).AddAttention(ATTENTION_INVALID_REGION);
    return true;
}

} // namespace

// TVoiceprintEnrollHandler ----------------------------------------------------

TVoiceprintEnrollHandler::TVoiceprintEnrollHandler(THolder<TBlackBoxAPI> blackBoxAPI,
                                                   THolder<TDataSyncAPI> dataSyncAPI)
    : BlackBoxAPI(std::move(blackBoxAPI))
    , DataSyncAPI(std::move(dataSyncAPI)) {
}

TResultValue TVoiceprintEnrollHandler::Do(TContext& ctx) {
    try {
        return DoImpl(ctx);
    } catch (const TInvalidParamException& e) {
        return TError{TError::EType::INVALIDPARAM, e.what()};
    }
}

TResultValue TVoiceprintEnrollHandler::DoImpl(TContext& ctx) {
    if (const auto error = IsValidBiometricsContext(ctx.ClientFeatures(), ctx.Meta())) {
        LOG(ERR) << error->ToJson() << Endl;
        // because on VINS's transition model after voiceprint_enroll one only can go to
        // voiceprint_enroll__collect_voice, and we do not want this in case of an error
        auto finish = ctx.SetResponseForm(ENROLL_FINISH, false /* setCurrentFormAsCallback */);
        Y_ENSURE(finish);
        auto* isErrorSlot = finish->GetOrCreateSlot(SLOT_IS_SERVER_ERROR, TYPE_BOOL);
        isErrorSlot->Value.SetBool(true);
        return ResultSuccess();
    }

    if (ShouldHaveBiometricsScores(ctx.ClientFeatures())) {
        CheckNeedLogin(ctx);
        CheckTooManyUsers(ctx, *BlackBoxAPI.Get(), *DataSyncAPI.Get());
        if (CheckInvalidRegion(ctx, true /* changeFormToEnrollFinish */)) {
            return ResultSuccess();
        }
        AddListeningPlayerPauseCommand(ctx);
        return ResultSuccess();
    }

    TContext::TPtr whatIsMyName = ctx.SetResponseForm(WHAT_IS_MY_NAME, false /* setCurrentFormAsCallback */);
    Y_ENSURE(whatIsMyName);
    SetSilentMode(*whatIsMyName);
    whatIsMyName->AddAttention(ATTENTION_ENROLL_REQUESTED);
    SetIsKnown(*whatIsMyName, false);

    TMaybe<TString> uid;
    TMaybe<TString> userName;

    if (const auto error = GetUidAndUsername(ctx, uid, userName, *BlackBoxAPI.Get(), *DataSyncAPI.Get())) {
        return error;
    }

    if (!uid.Defined()) {
        return ResultSuccess();
    }

    SetIsKnown(*whatIsMyName, userName.Defined());
    if (userName.Defined()) {
        auto* const userNameSlot = whatIsMyName->GetOrCreateSlot(SLOT_USER_NAME, TYPE_STRING);
        userNameSlot->Value.SetString(*userName.Get());
    }

    return ResultSuccess();
}

// TVoiceprintEnrollCollectVoiceHandler::TUserInfo -----------------------------
void TVoiceprintEnrollCollectVoiceHandler::TUserInfo::ToKeyValues(TVector<TPersonalDataHelper::TKeyValue>& kvs) const {
    kvs.emplace_back(TPersonalDataHelper::EUserSpecificKey::UserName, UserName);
    kvs.emplace_back(TPersonalDataHelper::EUserSpecificKey::Gender, Gender);

    if (GuestUID)
        kvs.emplace_back(TPersonalDataHelper::EUserSpecificKey::GuestUID, *GuestUID);
}

// TVoiceprintEnrollCollectVoiceHandler ----------------------------------------
// static
const TStringBuf TVoiceprintEnrollCollectVoiceHandler::GenderFemale = TStringBuf("female");
// static
const TStringBuf TVoiceprintEnrollCollectVoiceHandler::GenderMale = TStringBuf("male");
// static
const TStringBuf TVoiceprintEnrollCollectVoiceHandler::GenderUndefined = TStringBuf("undefined");

TVoiceprintEnrollCollectVoiceHandler::TVoiceprintEnrollCollectVoiceHandler(THolder<TBlackBoxAPI> blackBoxAPI,
                                                                           THolder<NAlice::TPassportAPI> passportAPI,
                                                                           THolder<TDataSyncAPI> dataSyncAPI)
    : BlackBoxAPI(std::move(blackBoxAPI))
    , PassportAPI(std::move(passportAPI))
    , DataSyncAPI(std::move(dataSyncAPI)) {
    Y_ASSERT(BlackBoxAPI);
    Y_ASSERT(PassportAPI);
    Y_ASSERT(DataSyncAPI);
}

TResultValue TVoiceprintEnrollCollectVoiceHandler::Do(TRequestHandler& r) {
    return Do(r.Ctx());
}

// static
TStringBuf TVoiceprintEnrollCollectVoiceHandler::DetectGender(TStringBuf phrase) {
    if (phrase.empty() || phrase == "unknown")
        return GenderUndefined;
    if (phrase == GenderMale || phrase == GenderFemale)
        return phrase;
    if (phrase.EndsWith("а"))
        return GenderFemale;
    return GenderMale;
}

TResultValue TVoiceprintEnrollCollectVoiceHandler::Do(TContext& ctx) {
    try {
        return DoImpl(ctx);
    } catch (const TInvalidParamException& e) {
        return TError{TError::EType::INVALIDPARAM, e.what()};
    }
}

TResultValue TVoiceprintEnrollCollectVoiceHandler::FreezeUserName(const TSlot* userNameSlot, TContext& ctx,
                                                                  TSlot*& userNameFrozenSlot) const {
    if (IsSlotEmptyString(userNameSlot) || IsSlotSwear(userNameSlot)) {
        ProcessUsernameError(ctx);
        return ResultSuccess();
    }
    auto userName = userNameSlot->Value.GetString();
    if (!userNameFrozenSlot) {
        userNameFrozenSlot = ctx.CreateSlot(SLOT_USER_NAME_FROZEN, TYPE_STRING);
    }
    userNameFrozenSlot->Value.SetString(userName);
    return ResultSuccess();
}

TResultValue TVoiceprintEnrollCollectVoiceHandler::Finish(const NSc::TValue& voiceRequests, TUserInfo& info,
                                                          TContext& ctx) {
    if (const auto error = IsValidBiometricsContext(ctx.ClientFeatures(), ctx.Meta())) {
        LOG(ERR) << error->ToJson() << Endl;
        return TError{TError::EType::SYSTEM, INVALID_BIO_CONFIG_MESSAGE};
    }

    const auto& meta = ctx.Meta();

    TString uid;

    if (!HasNonEmptyBiometricsScores(meta)) {
        Y_ASSERT(BlackBoxAPI);
        if (!BlackBoxAPI->GetUid(ctx, uid)) {
            SetError(ctx, "Error requesting UID from BlackBox API", false /* addErrorBlock */);
            return {TError{TError::EType::SYSTEM}};
        }

        // Even when there're no biometric scores, i.e. when the owner
        // of the station is being registered, we need to create a
        // guest user.
        const auto result = RegisterKolonkish(ctx, ctx, info.UserName);

        if (!result) {
            return {TError{TError::EType::SYSTEM}};
        }
        info.GuestUID = result->UID;
    } else {
        const auto result = RegisterKolonkish(ctx, ctx, info.UserName);

        if (!result) {
            return {TError{TError::EType::SYSTEM}};
        }
        uid = result->UID;
    }

    if (const auto error = SaveUserInfo(ctx, uid, info)) {
        SetDataSyncRequestError(ctx, error, false /* addErrorBlock */);
        return error;
    }

    TContext::TPtr finish = ctx.SetResponseForm(ENROLL_FINISH, false /* setCurrentFormAsCallback */);
    Y_ENSURE(finish);
    FillFinishForm(*finish, info.UserName, uid, {} /*code*/);

    AddSaveVoiceprintAction(*finish, uid, voiceRequests);

    return ResultSuccess();
}

TResultValue TVoiceprintEnrollCollectVoiceHandler::DoImpl(TContext& ctx) {
    if (const auto error = IsValidBiometricsContext(ctx.ClientFeatures(), ctx.Meta())) {
        LOG(ERR) << error->ToJson() << Endl;
        ProcessServerError(ctx);
    }

    const auto& meta = ctx.Meta();

    const auto* const userNameSlot = GetSlotTyped(ctx, SLOT_USER_NAME, NSc::TValue::EType::String);
    auto* readySlot = GetSlotTyped(ctx, SLOT_READY, NSc::TValue::EType::String);
    auto* userNameFrozenSlot = GetSlotTyped(ctx, SLOT_USER_NAME_FROZEN, NSc::TValue::EType::String);
    auto* readyFrozenSlot = GetSlotTyped(ctx, SLOT_READY_FROZEN, NSc::TValue::EType::String);
    auto* phrasesCountSlot = GetSlotTyped(ctx, SLOT_PHRASES_COUNT, NSc::TValue::EType::IntNumber);
    auto* userRepeatSlot = GetSlotTyped(ctx, SLOT_USER_REPEAT, NSc::TValue::EType::String);
    auto* isServerRepeatSlot = GetSlotTyped(ctx, SLOT_IS_SERVER_REPEAT, NSc::TValue::EType::Bool);
    auto* isNeedExplainSlot = GetSlotTyped(ctx, SLOT_IS_NEED_EXPLAIN, NSc::TValue::EType::Bool);

    if (CheckNeedLogin(ctx)) {
        return ResultSuccess();
    }

    if (CheckTooManyUsers(ctx, *BlackBoxAPI.Get(), *DataSyncAPI.Get())) {
        return ResultSuccess();
    }

    if (!IsSlotEmptyString(userRepeatSlot)) {
        NullifySlot(userRepeatSlot);
        return ResultSuccess();
    }

    NullifySlot(isServerRepeatSlot);
    NullifySlot(isNeedExplainSlot);

    auto* voiceRequestsSlot = GetSlotTyped(ctx, SLOT_VOICE_REQUESTS, NSc::TValue::EType::Array);
    if (!voiceRequestsSlot) {
        voiceRequestsSlot = ctx.CreateSlot(SLOT_VOICE_REQUESTS, TYPE_LIST);
    }
    NSc::TValue& voiceRequests = voiceRequestsSlot->Value;

    if (voiceRequests.IsNull()) {
        voiceRequests.SetArray();
    }

    if (IsSlotEmptyString(userNameFrozenSlot)) {
        NullifySlot(readySlot);
        if (IsSlotEmptyString(userNameSlot) || IsSlotSwear(userNameSlot)) {
            ProcessUsernameError(ctx);
            return ResultSuccess();
        }

        return FreezeUserName(userNameSlot, ctx, userNameFrozenSlot);
    }
    if (IsSlotEmptyString(readyFrozenSlot)) {
        if (IsSlotEmptyString(readySlot)) {
            return FreezeUserName(userNameSlot, ctx, userNameFrozenSlot);
        }
        auto ready = readySlot->Value.GetString();
        if (!readyFrozenSlot) {
            readyFrozenSlot = ctx.CreateSlot(SLOT_READY_FROZEN, TYPE_STRING);
        }
        readyFrozenSlot->Value.SetString(ready);
        if (!phrasesCountSlot) {
            phrasesCountSlot = ctx.CreateSlot(SLOT_PHRASES_COUNT, TYPE_INT);
        }
        phrasesCountSlot->Value.SetIntNumber(0);
        return ResultSuccess();
    }
    if (IsSlotEmpty(phrasesCountSlot) || !phrasesCountSlot->Value.IsIntNumber()) {
        return {TError{TError::EType::SYSTEM}};
    }
    if (HasBiometricsScores(meta) && meta.BiometricsScores().HasRequestId()) {
        voiceRequests.Push(NSc::TValue().SetString(meta.BiometricsScores().RequestId()));
    } else {
        ProcessServerError(ctx);
        return ResultSuccess();
    }
    auto phrasesCount = phrasesCountSlot->Value.GetIntNumber();
    ++phrasesCount;
    if (phrasesCount < TOTAL_ENROLL_PHRASES) {
        phrasesCountSlot->Value.SetIntNumber(phrasesCount);
        return ResultSuccess();
    }

    TUserInfo info;
    info.UserName = userNameFrozenSlot->Value.GetString();
    info.Gender = DetectGender(readyFrozenSlot->Value.GetString());
    if (const auto error = Finish(voiceRequests, info, ctx)) {
        voiceRequests.Pop();
        ProcessServerError(ctx);
        return ResultSuccess();
    }
    return ResultSuccess();
}

void TVoiceprintEnrollCollectVoiceHandler::FillFinishForm(TContext& ctx, TStringBuf userName, TMaybe<TString> uid,
                                                          TMaybe<TString> code) const {
    auto* const userNameSlot = ctx.GetOrCreateSlot(SLOT_USER_NAME, TYPE_STRING);
    Y_ASSERT(userNameSlot);
    userNameSlot->Value.SetString(userName);

    if (uid) {
        auto* const uidSlot = ctx.GetOrCreateSlot(SLOT_CREATED_UID, TYPE_STRING);
        Y_ASSERT(uidSlot);
        uidSlot->Value.SetString(*uid);
    }

    if (code) {
        auto* const codeSlot = ctx.GetOrCreateSlot(SLOT_CREATED_USER_SECRET, TYPE_STRING);
        Y_ASSERT(codeSlot);
        codeSlot->Value.SetString(*code);
    }
}

TResultValue TVoiceprintEnrollCollectVoiceHandler::SaveUserInfo(TContext& ctx, TStringBuf uid,
                                                                const TUserInfo& info) const {
    TVector<TPersonalDataHelper::TKeyValue> kvs;
    info.ToKeyValues(kvs);

    Y_ASSERT(DataSyncAPI);
    return DataSyncAPI->Save(ctx, uid, kvs);
}

TMaybe<NAlice::TPassportAPI::TResult>
TVoiceprintEnrollCollectVoiceHandler::RegisterKolonkish(TContext& ctx, TContext& finish, TStringBuf userName) const {
    Y_ASSERT(PassportAPI);
    const NAlice::TPassportAPI::TResult result = PassportAPI->RegisterKolonkish(
        ctx.GetSources().Passport().Request(), ctx.GetConfig().Vins().Passport().Consumer(),
        ctx.UserAuthorizationHeader(), ctx.UserIP());

    if (const auto& error = result.Error) {
        TStringBuilder message;
        message << "Error requesting Passport API, error type: " << error->Type;
        if (!error->Message.empty())
            message << ", error message: " << error->Message;

        LOG(ERR) << message << Endl;

        SetError(ctx, message, false /* addErrorBlock */);
        return Nothing();

        FillFinishForm(finish, userName, Nothing() /* uid */, Nothing() /* code */);
        SetError(finish, message, true /* addErrorBlock */);

        return Nothing();
    }

    return result;
}

// TWhatIsMyNameHandler --------------------------------------------------------
TWhatIsMyNameHandler::TWhatIsMyNameHandler(THolder<TBlackBoxAPI> blackBoxAPI, THolder<TDataSyncAPI> dataSyncAPI)
    : BlackBoxAPI(std::move(blackBoxAPI))
    , DataSyncAPI(std::move(dataSyncAPI)) {
}

TResultValue TWhatIsMyNameHandler::Do(TContext& ctx) {
    try {
        return DoImpl(ctx);
    } catch (const TInvalidParamException& e) {
        return TError{TError::EType::INVALIDPARAM, e.what()};
    }
}

TResultValue TWhatIsMyNameHandler::DoImpl(TContext& ctx) {
    TMaybe<TString> userName;

    if (const auto error = IsValidBiometricsContext(ctx.ClientFeatures(), ctx.Meta())) {
        LOG(ERR) << error->ToJson() << Endl;
        return TError{TError::EType::SYSTEM, INVALID_BIO_CONFIG_MESSAGE};
    }

    if (ShouldHaveBiometricsScores(ctx.ClientFeatures())) {
        TBiometryDelegate delegate{ctx, *BlackBoxAPI.Get(), *DataSyncAPI.Get()};
        TBiometry biometry{ctx.Meta(), delegate, TBiometry::EMode::HighTPR};
        TMaybe<TString> uid;
        if (const auto error = GetBiometryUidAndUsername(biometry, uid, userName)) {
            return error;
        }
        CheckInvalidRegion(ctx, false /* changeFormToEnrollFinish */);
    } else {
        SetSilentMode(ctx);
        TMaybe<TString> uid;

        if (const auto error = GetUidAndUsername(ctx, uid, userName, *BlackBoxAPI.Get(), *DataSyncAPI.Get())) {
            return error;
        }

        if (!uid.Defined()) {
            return ResultSuccess();
        }
    }
    if (AreTooManyUsers(ctx)) {
        if (!userName.Defined()) {
            ctx.GetOrCreateSlot(SLOT_IS_TOO_MANY_ENROLLED_USERS, TYPE_BOOL)->Value.SetBool(true);
        }
    }

    SetIsKnown(ctx, userName.Defined());
    if (userName.Defined()) {
        ctx.CreateSlot(SLOT_USER_NAME, TYPE_STRING, true /* optional */, NSc::TValue{*userName.Get()});
    }

    return ResultSuccess();
}

// TSetMyNameHandler -----------------------------------------------------------
TSetMyNameHandler::TSetMyNameHandler(THolder<TBlackBoxAPI> blackBoxAPI, THolder<TDataSyncAPI> dataSyncAPI)
    : BlackBoxAPI(std::move(blackBoxAPI))
    , DataSyncAPI(std::move(dataSyncAPI)) {
}

TResultValue TSetMyNameHandler::Do(TContext& ctx) {
    try {
        ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::IDENTITY_COMMANDS);
        return DoImpl(ctx);
    } catch (const TInvalidParamException& e) {
        return TError{TError::EType::INVALIDPARAM, e.what()};
    }
}

TResultValue TSetMyNameHandler::DoImpl(TContext& ctx) {
    auto* const distractorSlot = GetSlotTyped(ctx, SLOT_DISTRACTOR, NSc::TValue::EType::String);
    if (!IsSlotEmpty(distractorSlot)) {
        return ResultSuccess();
    }

    auto* const newUserNameSlot = GetSlotTyped(ctx, SLOT_USER_NAME, NSc::TValue::EType::String);
    Y_ASSERT(!IsSlotEmpty(newUserNameSlot) && newUserNameSlot->Value.IsString());
    TString newUserName;
    newUserName = newUserNameSlot->Value.GetString();

    TMaybe<TString> oldUserName;
    TMaybe<TString> uid;

    if (const auto error = IsValidBiometricsContext(ctx.ClientFeatures(), ctx.Meta())) {
        LOG(ERR) << error->ToJson() << Endl;
        return TError{TError::EType::SYSTEM, INVALID_BIO_CONFIG_MESSAGE};
    }

    if (IsSlotSwear(newUserNameSlot)) {
        return ResultSuccess();
    }

    if (ShouldHaveBiometricsScores(ctx.ClientFeatures())) {
        if (CheckNeedLogin(ctx)) {
            return ResultSuccess();
        }

        TBiometryDelegate delegate{ctx, *BlackBoxAPI.Get(), *DataSyncAPI.Get()};
        TBiometry biometry(ctx.Meta(), delegate, TBiometry::EMode::HighTPR);

        if (const auto error = GetBiometryUidAndUsername(biometry, uid, oldUserName)) {
            return error;
        }

        if (!uid.Defined()) {
            if (CheckTooManyUsers(ctx, *BlackBoxAPI.Get(), *DataSyncAPI.Get())) {
                GetActualResponseForm(ctx).GetOrCreateSlot(SLOT_IS_CHANGE_NAME, TYPE_BOOL)->Value.SetBool(true);
                return ResultSuccess();
            }

            if (CheckInvalidRegion(ctx, true /* changeFormToEnrollFinish */)) {
                return ResultSuccess();
            }

            TContext::TPtr collect = ctx.SetResponseForm(ENROLL_COLLECT_VOICE, false /* setCurrentFormAsCallback */);
            Y_ENSURE(collect);
            AddListeningPlayerPauseCommand(*collect);
            collect->GetOrCreateSlot(SLOT_USER_NAME, TYPE_STRING)->Value.SetString(newUserName);

            collect->GetOrCreateSlot(SLOT_USER_NAME_FROZEN, TYPE_STRING)->Value.SetString(newUserName);
            collect->GetOrCreateSlot(SLOT_IS_NEED_EXPLAIN, TYPE_BOOL)->Value.SetBool(true);

            return ResultSuccess();
        }
    } else {
        SetSilentMode(ctx);

        if (const auto error = GetUidAndUsername(ctx, uid, oldUserName, *BlackBoxAPI.Get(), *DataSyncAPI.Get())) {
            return error;
        }

        if (!uid.Defined()) {
            return ResultSuccess();
        }
    }

    Y_ASSERT(uid.Defined());

    if (oldUserName.Defined() && *oldUserName.Get() != newUserName) {
        SetOldUsername(ctx, *oldUserName.Get());
    }

    return SaveNewUserName(ctx, *uid.Get(), newUserName);
}

TResultValue TSetMyNameHandler::SaveNewUserName(TContext& ctx, TString uid, TString newUserName) {
    TVector<TPersonalDataHelper::TKeyValue> kvs;
    kvs.emplace_back(TPersonalDataHelper::EUserSpecificKey::UserName, newUserName);

    if (const auto error = DataSyncAPI->Save(ctx, uid, kvs)) {
        SetDataSyncRequestError(ctx, error, true /* addErrorBlock */);
        return ResultSuccess();
    }
    return ResultSuccess();
}

void TSetMyNameHandler::SetOldUsername(TContext& ctx, TString oldUserName) {
    auto* const oldUserNameSlot = ctx.GetOrCreateSlot(SLOT_OLD_USER_NAME, TYPE_STRING);
    oldUserNameSlot->Value.SetString(oldUserName);
}

// -----------------------------------------------------------------------------
void RegisterVoiceprintEnrollHandlers(THandlersMap& handlers) {
    handlers.emplace(ENROLL, [] {
        return MakeHolder<TVoiceprintEnrollHandler>(MakeHolder<TBlackBoxAPI>(), MakeHolder<TDataSyncAPI>());
    });
    handlers.emplace(ENROLL_COLLECT_VOICE, [] {
        return MakeHolder<TVoiceprintEnrollCollectVoiceHandler>(MakeHolder<TBlackBoxAPI>(), MakeHolder<NAlice::TPassportAPI>(),
                                                        MakeHolder<TDataSyncAPI>());
    });
    handlers.emplace(ENROLL_FINISH, [] { return MakeHolder<TEchoHandler>(); });
    handlers.emplace(WHAT_IS_MY_NAME, [] {
        return MakeHolder<TWhatIsMyNameHandler>(MakeHolder<TBlackBoxAPI>(), MakeHolder<TDataSyncAPI>());
    });
    handlers.emplace(SET_MY_NAME, [] {
        return MakeHolder<TSetMyNameHandler>(MakeHolder<TBlackBoxAPI>(), MakeHolder<TDataSyncAPI>());
    });
}
} // namespace NBASS
