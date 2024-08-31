package ru.yandex.alice.paskill.dialogovo.external.v1.response

import com.fasterxml.jackson.annotation.JsonProperty
import com.fasterxml.jackson.annotation.JsonRawValue
import com.fasterxml.jackson.databind.annotation.JsonDeserialize
import ru.yandex.alice.kronstadt.core.utils.AnythingToStringJacksonDeserializer
import ru.yandex.alice.paskill.dialogovo.domain.Censored
import ru.yandex.alice.paskill.dialogovo.utils.MdsImageId
import ru.yandex.alice.paskill.dialogovo.utils.SizeInBytes
import javax.annotation.Nullable
import javax.validation.Valid
import javax.validation.constraints.Size

data class TeaserMeta(
    @field:Nullable
    @field:Censored
    @field:Size(max = 128)
    val title: String?,

    @Nullable
    @field:Censored
    @field:Size(max = 512)
    val text: String?,

    @Nullable
    @field:MdsImageId
    @JsonProperty("image_id")
    val imageId: String?,

    @JsonProperty("tap_action")
    val tapAction: @Valid TapAction?
) {
    data class TapAction(
        @field:SizeInBytes(max = 4096)
        @JsonRawValue
        @JsonDeserialize(using = AnythingToStringJacksonDeserializer::class)
        @Nullable
        val payload: String?,

        @Nullable
        @field:Size(max = 1024)
        @JsonProperty("activation_command") val activationCommand: String?
    )
}
