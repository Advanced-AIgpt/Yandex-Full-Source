package ru.yandex.alice.paskill.dialogovo.service.recommender

import com.fasterxml.jackson.annotation.JsonIgnoreProperties
import com.fasterxml.jackson.annotation.JsonProperty
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo.Look

@JsonIgnoreProperties(ignoreUnknown = true)
data class RecommenderItem(
    @JsonProperty("id") val skillId: String,
    val activation: String,
    val description: String,
    @JsonProperty("logo_avatar_id") val logoAvatarId: String?,
    @JsonProperty("logo_prefix") val logoPrefix: String,
    val look: Look,
    val name: String,
    @JsonProperty("logo_bg_color") val logoBgColor: String?,
    @JsonProperty("logo_fg_round_image_url") val logoFgRoundImageUrl: String?,
    @JsonProperty("logo_amelie_fg_url") val logoAmelieFgUrl: String?,
    @JsonProperty("logo_amelie_bg_url") val logoAmelieBgUrl: String?,
    @JsonProperty("logo_amelie_bg_wide_url") val logoAmelieBgWideUrl: String?,
    @JsonProperty("search_app_card_item_text") val searchAppCardItemText: String?
)
