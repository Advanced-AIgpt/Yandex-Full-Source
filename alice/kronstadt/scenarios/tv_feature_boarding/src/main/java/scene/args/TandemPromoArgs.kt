package ru.yandex.alice.kronstadt.scenarios.tv_feature_boarding.scene.args

data class TandemPromoArgs(
    val isTandemDevicesAvailable: Boolean,
    val isTandemConnected: Boolean,
    val templateName: String,
    val lastShowTime: ULong,
    val showCount: UInt
)
