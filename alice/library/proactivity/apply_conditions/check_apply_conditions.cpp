#include "check_apply_conditions.h"

#include <util/generic/algorithm.h>

namespace NAlice {

TMaybe<bool> CheckApplyConditionResponseComponentIsOk(const NDJ::NAS::TApplyCondition::EResponseVoiceOrText condition,
                                                      const bool postrollHasComponent, const bool responseHasComponent) {
    switch (condition) {
        case NDJ::NAS::TApplyCondition::MatchResult:
            return !postrollHasComponent || responseHasComponent;
        case NDJ::NAS::TApplyCondition::IsPresent:
            return responseHasComponent;
        case NDJ::NAS::TApplyCondition::IsAbsent:
            return !responseHasComponent;
        case NDJ::NAS::TApplyCondition::Ignore:
            return true;
        default:
            return Nothing();
    }
}

bool CheckApplyConditionResponseDirectiveIsOk(const NDJ::NAS::TApplyCondition::TDirectiveCondition& cond, 
                                              const google::protobuf::RepeatedPtrField<NAlice::NSpeechKit::TDirective>& directives) {
    const bool directiveIsPresent = AnyOf(directives, [&cond](const auto& directive) {
        return directive.GetName() == cond.GetName();
    });
    const bool directiveShouldBePresent = cond.GetStatus() == NDJ::NAS::TApplyCondition_TDirectiveCondition_EDirectiveStatus_IsPresent;
    return directiveIsPresent == directiveShouldBePresent;
}

} // NAlice
