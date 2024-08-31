package ru.yandex.alice.kronstadt.scenarios.tv_channels.model

import com.fasterxml.jackson.annotation.JsonProperty

data class PaidChannelStub(
    @JsonProperty("paid_channel_stub_body_text") val bodyText: List<String>,
    @JsonProperty("paid_channel_stub_header_image") val headerImage: String,
)
