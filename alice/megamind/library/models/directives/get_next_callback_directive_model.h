#pragma once

#include "callback_directive_model.h"
#include "server_directive_model.h"

#include <util/generic/maybe.h>

namespace NAlice::NMegamind {

class TGetNextCallbackDirectiveModel final : public virtual TServerDirectiveModel {
public:
    TGetNextCallbackDirectiveModel(bool ignoreAnswer,
                                   bool isLedSilent,
                                   const TString& sessionId,
                                   const TString& productScenarioName,
                                   TMaybe<NMegamind::TCallbackDirectiveModel> recoveryCallback = Nothing(),
                                   TMaybe<TString> multiroomSessionId = Nothing(),
                                   TMaybe<TString> roomId = Nothing());
    TGetNextCallbackDirectiveModel(TGetNextCallbackDirectiveModel&& callbackDirectiveModel);

    void Accept(IModelSerializer& serializer) const final;

    [[nodiscard]] bool GetIsLedSilent() const;
    [[nodiscard]] const TString& GetSessionId() const;
    [[nodiscard]] const TString& GetProductScenarioName() const;
    [[nodiscard]] const TMaybe<TCallbackDirectiveModel>& GetRecoveryCallback() const;

private:
    bool IsLedSilent;
    TString SessionId;
    TString ProductScenarioName;
    TMaybe<TCallbackDirectiveModel> RecoveryCallback;
};

} // namespace NAlice::NMegamind
