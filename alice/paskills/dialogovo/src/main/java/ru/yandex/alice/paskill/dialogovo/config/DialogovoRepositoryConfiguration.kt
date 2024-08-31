package ru.yandex.alice.paskill.dialogovo.config

import org.springframework.context.annotation.Configuration
import org.springframework.data.jdbc.repository.config.EnableJdbcRepositories

@Configuration
@EnableJdbcRepositories(
    basePackages = [
        "ru.yandex.alice.paskill.dialogovo.providers.skill",
        "ru.yandex.alice.paskill.dialogovo.scenarios.news.providers"
    ]
)
open class DialogovoRepositoryConfiguration
