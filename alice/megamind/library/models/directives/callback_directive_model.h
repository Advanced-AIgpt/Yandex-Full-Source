#pragma once

#include "server_directive_model.h"

#include <google/protobuf/struct.pb.h>

#include <util/generic/maybe.h>

namespace NAlice::NMegamind {

class TCallbackDirectiveModel final : public virtual TServerDirectiveModel {
public:
    TCallbackDirectiveModel(const TString& name,
                            bool ignoreAnswer,
                            google::protobuf::Struct payload,
                            bool isLedSilent,
                            TMaybe<TString> multiroomSessionId = Nothing(),
                            TMaybe<TString> roomId = Nothing(),
                            TMaybe<NScenarios::TLocationInfo> locationInfo = Nothing());
    TCallbackDirectiveModel(TCallbackDirectiveModel&& callbackDirectiveModel);

    void Accept(IModelSerializer& serializer) const final;

    [[nodiscard]] const google::protobuf::Struct& GetPayload() const;
    [[nodiscard]] bool GetIsLedSilent() const;

private:
    google::protobuf::Struct Payload;
    bool IsLedSilent;
};

} // namespace NAlice::NMegamind
