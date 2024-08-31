#pragma once

#include <alice/megamind/library/modifiers/context.h>
#include <alice/megamind/library/response/response.h>
#include <alice/megamind/library/speechkit/request.h>

#include <alice/megamind/protos/modifiers/modifiers.pb.h>
#include <alice/megamind/protos/proactivity/proactivity.pb.h>

#include <alice/library/proactivity/success_conditions/match_success_conditions.h>

#include <dj/lib/proto/action.pb.h>
#include <dj/services/alisa_skills/profile/proto/profile_enums.pb.h>
#include <dj/services/alisa_skills/server/proto/data/data_types.pb.h>

namespace NAlice::NMegamind {

NDJ::TActionProto MakePostrollAction(const TSpeechKitRequest& skr, const NDJ::NAS::TProtoItem& item,
                                     NDJ::NAS::EAlisaSkillsActionType actionType, const TString& source,
                                     const TString& postrollViewRequestId = "",
                                     const NDJ::NAS::TPostrollContext* proactivityContext = nullptr);

NDJ::NAS::TProtoItem PostrollViewToProtoItem(const TProactivityStorage::TPostrollView& postrollView);

bool CheckCurrentScenarioForClickAction(TResponseModifierContext& ctx, const TScenarioResponse& response);

void CheckCurrentScenarioForDeclineAction(TResponseModifierContext& ctx, const TScenarioResponse& response);

} // NAlice::NMegamind
