#include "get_next_callback_directive_model.h"

#include <alice/megamind/library/request/event/server_action_event.h>

#include <google/protobuf/util/json_util.h>

#include <utility>

namespace NAlice::NMegamind {

TGetNextCallbackDirectiveModel::TGetNextCallbackDirectiveModel(bool ignoreAnswer,
                                                               bool isLedSilent,
                                                               const TString& sessionId,
                                                               const TString& productScenarioName,
                                                               TMaybe<NMegamind::TCallbackDirectiveModel> recoveryCallback,
                                                               TMaybe<TString> multiroomSessionId,
                                                               TMaybe<TString> roomId)
    : TServerDirectiveModel(MM_STACK_ENGINE_GET_NEXT_CALLBACK_NAME, ignoreAnswer, multiroomSessionId, roomId)
    , IsLedSilent(isLedSilent)
    , SessionId(sessionId)
    , ProductScenarioName(productScenarioName)
    , RecoveryCallback(std::move(recoveryCallback))
{
}

TGetNextCallbackDirectiveModel::TGetNextCallbackDirectiveModel(TGetNextCallbackDirectiveModel&& getNextCallbackDirectiveModel)
    : TServerDirectiveModel(getNextCallbackDirectiveModel.GetName(),
                            getNextCallbackDirectiveModel.GetIgnoreAnswer(),
                            getNextCallbackDirectiveModel.GetMultiroomSessionId(),
                            getNextCallbackDirectiveModel.GetRoomId())
    , IsLedSilent(getNextCallbackDirectiveModel.GetIsLedSilent())
    , SessionId(getNextCallbackDirectiveModel.GetSessionId())
    , ProductScenarioName(getNextCallbackDirectiveModel.GetProductScenarioName())
    , RecoveryCallback(std::move(getNextCallbackDirectiveModel.RecoveryCallback))
{
}

void TGetNextCallbackDirectiveModel::Accept(IModelSerializer& serializer) const {
    serializer.Visit(*this);
}

bool TGetNextCallbackDirectiveModel::GetIsLedSilent() const {
    return IsLedSilent;
}

const TString& TGetNextCallbackDirectiveModel::GetSessionId() const {
    return SessionId;
}

const TString& TGetNextCallbackDirectiveModel::GetProductScenarioName() const {
    return ProductScenarioName;
}

const TMaybe<TCallbackDirectiveModel>& TGetNextCallbackDirectiveModel::GetRecoveryCallback() const {
    return RecoveryCallback;
}

} // namespace NAlice::NMegamind
