package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives

import com.fasterxml.jackson.annotation.JsonProperty
import ru.yandex.alice.kronstadt.core.directive.Directive

@Directive("external_skill__account_linking_complete")
data class AccountLinkingCompleteDirective(
    @JsonProperty("skill_id") override val skillId: String,
    @JsonProperty("initial_device_id") override val initialDeviceId: String?,
) : ReturnToDeviceCallbackDirective
