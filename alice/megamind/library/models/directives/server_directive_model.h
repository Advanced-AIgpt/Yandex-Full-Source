#pragma once

#include <alice/megamind/library/models/interfaces/directive_model.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>

#include <util/generic/maybe.h>

namespace NAlice::NMegamind {

class TServerDirectiveModel : public TBaseDirectiveModel {
public:
    TServerDirectiveModel(const TString& name, bool ignoreAnswer,
                          TMaybe<TString> multiroomSessionId = Nothing(),
                          TMaybe<TString> roomId = Nothing(),
                          TMaybe<NScenarios::TLocationInfo> locationInfo = Nothing());

    [[nodiscard]] const TString& GetName() const final;
    [[nodiscard]] bool GetIgnoreAnswer() const;
    [[nodiscard]] EDirectiveType GetType() const final;
    [[nodiscard]] const TMaybe<TString>& GetMultiroomSessionId() const;
    [[nodiscard]] const TMaybe<TString>& GetRoomId() const;
    [[nodiscard]] const TMaybe<NScenarios::TLocationInfo>& GetLocationInfo() const;

private:
    TString Name;
    bool IgnoreAnswer;
    TMaybe<TString> MultiroomSessionId;
    TMaybe<TString> RoomId;
    TMaybe<NScenarios::TLocationInfo> LocationInfo;
};

} // namespace NAlice::NMegamind
