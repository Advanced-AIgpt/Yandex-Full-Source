package ru.yandex.alice.kronstadt.scenarios.tvmain

data class TvContext(
    val deviceId: String,
    val parentScreenId: String?,
    val screenId: String,
    val offset: Int,
    val limit: Int,
    val requestId: String,
    val apphostRequestId: String,
)

data class CarouselContext(
    val title: String,
    val carouselId: String,
    val carouselPosition: Int,
    val tvContext: TvContext
)

data class CardContext(val carouselContext: CarouselContext, val cardPosition: Int)
