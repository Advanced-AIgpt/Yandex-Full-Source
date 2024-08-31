package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives

import com.fasterxml.jackson.annotation.JsonProperty
import ru.yandex.alice.kronstadt.core.directive.Directive

@Directive("external_skill__user_agreements_accepted")
data class UserAgreementsAcceptedDirective(
    @JsonProperty("skill_id") override val skillId: String,
    @JsonProperty("user_agreement_ids") val userAgreementIds: Set<String>,
    @JsonProperty("initial_device_id") override val initialDeviceId: String?,
) : ReturnToDeviceCallbackDirective {
    constructor(skillId: String, userAgreementIds: List<String>, initialDeviceId: String?) : this(
        skillId,
        userAgreementIds.toSet(),
        initialDeviceId
    )
}

@Directive("external_skill__user_agreements_rejected")
data class UserAgreementsRejectedDirective(
    @JsonProperty("skill_id") override val skillId: String,
    @JsonProperty("initial_device_id") override val initialDeviceId: String?,
) : ReturnToDeviceCallbackDirective
