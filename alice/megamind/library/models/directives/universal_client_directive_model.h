#pragma once

#include <alice/megamind/library/models/directives/client_directive_model.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>

#include <google/protobuf/struct.pb.h>

#include <util/generic/maybe.h>

namespace NAlice::NMegamind {

class TUniversalClientDirectiveModel final : public TClientDirectiveModel {
public:
    TUniversalClientDirectiveModel(const TString& name,
                                   const TString& analyticsType,
                                   google::protobuf::Struct payload,
                                   TMaybe<TString> multiroomSessionId = Nothing(),
                                   TMaybe<TString> roomId = Nothing(),
                                   TMaybe<NScenarios::TLocationInfo> locationInfo = Nothing());

    void Accept(IModelSerializer& serializer) const final;

    const google::protobuf::Struct& GetPayload() const;
    const TMaybe<TString>& GetMultiroomSessionId() const;
    const TMaybe<TString>& GetRoomId() const;
    const TMaybe<NScenarios::TLocationInfo>& GetLocationInfo() const;

private:
    google::protobuf::Struct Payload;
    TMaybe<TString> MultiroomSessionId;
    TMaybe<TString> RoomId;
    const TMaybe<NScenarios::TLocationInfo> LocationInfo;
};

} // namespace NAlice::NMegamind
