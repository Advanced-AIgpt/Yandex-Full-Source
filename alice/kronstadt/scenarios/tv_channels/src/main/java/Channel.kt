package ru.yandex.alice.kronstadt.scenarios.tv_channels

import javax.validation.constraints.NotBlank

data class Channel(
    @NotBlank
    val uri: String,
    @NotBlank
    val name: String,
    val number: Int,
)
