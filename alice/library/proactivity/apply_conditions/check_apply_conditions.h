#pragma once

#include <alice/megamind/protos/speechkit/directives.pb.h>
#include <dj/services/alisa_skills/server/proto/data/data_types.pb.h>

#include <util/generic/maybe.h>

namespace NAlice {

TMaybe<bool> CheckApplyConditionResponseComponentIsOk(const NDJ::NAS::TApplyCondition::EResponseVoiceOrText condition, 
                                                      const bool postrollHasComponent, const bool responseHasComponent);

bool CheckApplyConditionResponseDirectiveIsOk(const NDJ::NAS::TApplyCondition::TDirectiveCondition& cond, 
                                              const google::protobuf::RepeatedPtrField<NAlice::NSpeechKit::TDirective>& directives);

} // NAlice
