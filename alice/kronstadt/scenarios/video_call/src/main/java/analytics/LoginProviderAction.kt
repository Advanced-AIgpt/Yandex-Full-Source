package ru.yandex.alice.kronstadt.scenarios.video_call.analytics

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoAction

object LoginProviderAction: AnalyticsInfoAction(
    "call.video_call_provider_login",
    "login to telegram",
    "Совершается вход в телеграм аккаунт"
)