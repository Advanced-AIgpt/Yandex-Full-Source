package ru.yandex.alice.kronstadt.core.directive

import com.fasterxml.jackson.annotation.JsonProperty

data class Style(
    @JsonProperty("suggest_border_color")
    val suggestBorderColor: String,

    @JsonProperty("oknyx_error_colors")
    val oknyxErrorColors: List<String>,

    @JsonProperty("user_bubble_fill_color")
    val userBubbleFillColor: String,

    @JsonProperty("suggest_text_color")
    val suggestTextColor: String,

    @JsonProperty("suggest_fill_color")
    val suggestFillColor: String,

    @JsonProperty("user_bubble_text_color")
    val userBubbleTextColor: String,

    @JsonProperty("oknyx_normal_colors")
    val oknyxNormalColors: List<String>,

    @JsonProperty("skill_actions_text_color")
    val skillActionsTextColor: String,

    @JsonProperty("skill_bubble_fill_color")
    val skillBubbleFillColor: String,

    @JsonProperty("skill_bubble_text_color")
    val skillBubbleTextColor: String,

    @JsonProperty("oknyx_logo")
    val oknyxLogo: String,
)
