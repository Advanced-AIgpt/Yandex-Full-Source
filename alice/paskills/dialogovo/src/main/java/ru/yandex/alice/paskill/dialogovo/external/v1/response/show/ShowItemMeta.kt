package ru.yandex.alice.paskill.dialogovo.external.v1.response.show

import com.fasterxml.jackson.annotation.JsonInclude
import com.fasterxml.jackson.annotation.JsonProperty
import lombok.Builder
import ru.yandex.alice.paskill.dialogovo.domain.Censored
import ru.yandex.alice.paskill.dialogovo.utils.validator.SizeWithoutTags
import java.time.Instant
import javax.validation.constraints.NotBlank
import javax.validation.constraints.NotNull
import javax.validation.constraints.Past
import javax.validation.constraints.Size

@Builder
@Censored
@JsonInclude(JsonInclude.Include.NON_ABSENT)
data class ShowItemMeta(
    @JsonProperty("content_id")
    @field:Size(max = 64)
    @field:NotNull
    @field:NotBlank
    val id: String,

    @field:Censored
    @field:Size(max = 256)
    val title: String? = null,

    @JsonProperty("title_tts")
    @field:Censored(isVoice = true)
    @field:SizeWithoutTags(
        max = 256,
        message = "размер не должен превышать {max} без учета тегов"
    )
    val titleTts: String? = null,

    @JsonProperty("publication_date")
    @field:NotNull
    @field:Past
    val publicationDate: Instant,

    @JsonProperty("expiration_date")
    val expirationDate: Instant? = null
)
