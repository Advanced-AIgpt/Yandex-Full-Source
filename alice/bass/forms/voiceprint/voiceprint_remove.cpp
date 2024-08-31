// Code in this file implements logic described here:
// https://wiki.yandex-team.ru/assistant/dialogs/voiceprint-remove/

#include "voiceprint_remove.h"

#include "voiceprint_utils.h"

#include <alice/bass/forms/common/biometry_delegate.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/util/error.h>

#include <alice/library/analytics/common/product_scenarios.h>

#include <util/generic/string.h>
#include <util/system/yassert.h>

namespace NBASS {

namespace {

constexpr TStringBuf ACTION_REMOVE_VOICEPRINT = "remove_voiceprint";
constexpr TStringBuf ATTENTION_SERVER_ERROR = "server_error";
constexpr TStringBuf ATTENTION_BIOMETRY_GUEST = "biometry_guest";

constexpr TStringBuf FORM_REMOVE = "personal_assistant.scenarios.voiceprint_remove";
constexpr TStringBuf FORM_REMOVE_CONFIRM = "personal_assistant.scenarios.voiceprint_remove__confirm";
constexpr TStringBuf FORM_REMOVE_FINISH = "personal_assistant.scenarios.voiceprint_remove__finish";

constexpr TStringBuf SLOT_CONFIRM = "confirm";
constexpr TStringBuf SLOT_IS_REMOVED = "is_removed";
constexpr TStringBuf SLOT_IS_NO_USERS = "is_no_users";
constexpr TStringBuf SLOT_UID = "uid";
constexpr TStringBuf SLOT_USER_NAME = "user_name";

constexpr TStringBuf TYPE_BOOL = "bool";
constexpr TStringBuf TYPE_STRING = "string";

void FinishWithError(TContext& ctx) {
    auto finish = ctx.SetResponseForm(FORM_REMOVE_FINISH, false /* setCurrentFormAsCallback */);
    Y_ENSURE(finish);
    finish->AddAttention(ATTENTION_SERVER_ERROR);
}

} // namespace

TVoiceprintRemoveHandler::TVoiceprintRemoveHandler(THolder<TBlackBoxAPI> blackBoxAPI,
                                                   THolder<TDataSyncAPI> dataSyncAPI)
    : BlackBoxAPI(std::move(blackBoxAPI))
    , DataSyncAPI(std::move(dataSyncAPI)) {
}

TResultValue TVoiceprintRemoveHandler::DoImpl(TContext& ctx) {
    using namespace NAlice::NBiometry;

    if (const auto error = IsValidBiometricsContext(ctx.ClientFeatures(), ctx.Meta())) {
        LOG(ERR) << error->ToJson() << Endl;
        FinishWithError(ctx);
        return ResultSuccess();
    }

    TBiometryDelegate delegate{ctx, *BlackBoxAPI.Get(), *DataSyncAPI.Get()};
    TBiometry noGuestBiometry(ctx.Meta(), delegate, TBiometry::EMode::NoGuest);
    if (!noGuestBiometry.IsKnownUser()) {
        auto finish = ctx.SetResponseForm(FORM_REMOVE_FINISH, false /* setCurrentFormAsCallback */);
        Y_ENSURE(finish);
        finish->GetOrCreateSlot(SLOT_IS_NO_USERS, TYPE_BOOL)->Value.SetBool(true);
        return ResultSuccess();
    }
    TString uid;
    if (const auto result = noGuestBiometry.GetUserId(uid)) {
        LOG(ERR) << "Error geting user id: " << result->ToJson() << Endl;
        FinishWithError(ctx);
        return ResultSuccess();
    }

    TString userName;
    bool validUserName = true;
    if (const auto result = noGuestBiometry.GetUserName(userName)) {
        validUserName = false;
        LOG(ERR) << "Error geting user name: " << result->ToJson() << Endl;
    }

    auto confirm = ctx.SetResponseForm(FORM_REMOVE_CONFIRM, false /* setCurrentFormAsCallback */);
    Y_ENSURE(confirm);
    confirm->GetOrCreateSlot(SLOT_UID, TYPE_STRING)->Value.SetString(uid);

    if (validUserName) {
        confirm->GetOrCreateSlot(SLOT_USER_NAME, TYPE_STRING)->Value.SetString(userName);
    }

    TBiometry biometry(ctx.Meta(), delegate, TBiometry::EMode::MaxAccuracy);
    if (!biometry.IsKnownUser()) {
        confirm->AddAttention(ATTENTION_BIOMETRY_GUEST);
    }

    return ResultSuccess();
}

TResultValue TVoiceprintRemoveConfirmHandler::Do(TContext& ctx) {
    try {
        ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::IDENTITY_COMMANDS);
        return DoImpl(ctx);
    } catch (const TInvalidParamException& e) {
        // TODO: perhaps we should use FinishWithError here too.
        return TError{TError::EType::INVALIDPARAM, e.what()};
    }
}

TResultValue TVoiceprintRemoveConfirmHandler::DoImpl(TContext& ctx) {
    using namespace NAlice::NBiometry;

    if (const auto error = IsValidBiometricsContext(ctx.ClientFeatures(), ctx.Meta())) {
        LOG(ERR) << error->ToJson() << Endl;
        FinishWithError(ctx);
        return ResultSuccess();
    }

    auto confirmSlot = GetSlotTyped(ctx, SLOT_CONFIRM, NSc::TValue::EType::String);
    auto uidSlot = GetSlotTyped(ctx, SLOT_UID, NSc::TValue::EType::String);
    auto finish = ctx.SetResponseForm(FORM_REMOVE_FINISH, false /* setCurrentFormAsCallback */);
    Y_ENSURE(finish);

    if (IsSlotEmpty(confirmSlot)) {
        finish->GetOrCreateSlot(SLOT_IS_REMOVED, TYPE_BOOL)->Value.SetBool(false);
        return ResultSuccess();
    }
    if (IsSlotEmpty(uidSlot)) {
        LOG(ERR) << "Uid slot is empty" << Endl;
        finish->AddAttention(ATTENTION_SERVER_ERROR);
        return ResultSuccess();
    }

    NSc::TValue data;
    data["user_id"] = uidSlot->Value.GetString();
    finish->AddUniProxyAction(ACTION_REMOVE_VOICEPRINT, std::move(data));
    finish->GetOrCreateSlot(SLOT_IS_REMOVED, TYPE_BOOL)->Value.SetBool(true);
    return ResultSuccess();
}

void RegisterVoiceprintRemoveHandlers(THandlersMap& handlers) {
    handlers.emplace(FORM_REMOVE, [] {
        return MakeHolder<TVoiceprintRemoveHandler>(MakeHolder<TBlackBoxAPI>(), MakeHolder<TDataSyncAPI>());
    });

    handlers.emplace(FORM_REMOVE_CONFIRM, [] { return MakeHolder<TVoiceprintRemoveConfirmHandler>(); });
    handlers.emplace(FORM_REMOVE_FINISH, [] { return MakeHolder<TEchoHandler>(); });
}

} // namespace NBASS
