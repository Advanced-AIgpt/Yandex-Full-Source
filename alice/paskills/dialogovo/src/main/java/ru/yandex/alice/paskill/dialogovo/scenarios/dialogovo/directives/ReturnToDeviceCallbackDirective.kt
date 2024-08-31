package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives

import com.fasterxml.jackson.annotation.JsonProperty
import ru.yandex.alice.kronstadt.core.directive.CallbackDirective

/**
 * A CallbackDirective that can be sent to a device. Used in cross-device scenarios
 * to transfer data between smart speakers and Yandex app.
 */
interface ReturnToDeviceCallbackDirective : CallbackDirective {
    @get:JsonProperty("skill_id")
    val skillId: String
    @get:JsonProperty("initial_device_id")
    val initialDeviceId: String?
}
