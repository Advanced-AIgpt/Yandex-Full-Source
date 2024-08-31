package ru.yandex.alice.paskill.dialogovo.vins

class ExternalDevError @JvmOverloads constructor(
    val type: String,
    val message: String,
    val path: String? = null,
    val details: String? = null,
)
