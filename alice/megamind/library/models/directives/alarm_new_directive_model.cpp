#include "alarm_new_directive_model.h"

#include <utility>

namespace NAlice::NMegamind {

TAlarmNewDirectiveModel::TAlarmNewDirectiveModel(const TString& analyticsType, const TString& state,
                                                 google::protobuf::Struct onSuccessCallbackPayload,
                                                 google::protobuf::Struct onFailureCallbackPayload)
    : TClientDirectiveModel("alarm_new", analyticsType)
    , State(state)
    , OnSuccessCallbackPayload(std::move(onSuccessCallbackPayload))
    , OnFailureCallbackPayload(std::move(onFailureCallbackPayload)) {
}

void TAlarmNewDirectiveModel::Accept(IModelSerializer& serializer) const {
    serializer.Visit(*this);
}

const TString& TAlarmNewDirectiveModel::GetState() const {
    return State;
}

const google::protobuf::Struct& TAlarmNewDirectiveModel::GetOnSuccessCallbackPayload() const {
    return OnSuccessCallbackPayload;
}

const google::protobuf::Struct& TAlarmNewDirectiveModel::GetOnFailureCallbackPayload() const {
    return OnFailureCallbackPayload;
}

} // namespace NAlice::NMegamind
