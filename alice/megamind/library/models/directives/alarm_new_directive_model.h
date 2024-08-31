#pragma once

#include "client_directive_model.h"

#include <google/protobuf/struct.pb.h>

namespace NAlice::NMegamind {

class TAlarmNewDirectiveModel final : public TClientDirectiveModel {
public:
    TAlarmNewDirectiveModel(const TString& analyticsType, const TString& state,
                            google::protobuf::Struct onSuccessCallbackPayload,
                            google::protobuf::Struct onFailureCallbackPayload);

    void Accept(IModelSerializer& serializer) const final;

    [[nodiscard]] const TString& GetState() const;

    [[nodiscard]] const google::protobuf::Struct& GetOnSuccessCallbackPayload() const;
    [[nodiscard]] const google::protobuf::Struct& GetOnFailureCallbackPayload() const;

private:
    const TString State;
    const google::protobuf::Struct OnSuccessCallbackPayload;
    const google::protobuf::Struct OnFailureCallbackPayload;
};

} // namespace NAlice::NMegamind
