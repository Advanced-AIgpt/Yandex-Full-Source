package ru.yandex.alice.kronstadt.scenarios.video_call.analytics

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoAction

object DiscardVideoCallAction: AnalyticsInfoAction(
    "call.discard_video_call",
    "discard call from telegram",
    "Отклоняется звонок из телеграма"
)