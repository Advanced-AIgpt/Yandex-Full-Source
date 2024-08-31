package ru.yandex.alice.paskill.dialogovo.scenarios.theremin

import org.springframework.context.annotation.Configuration
import org.springframework.data.jdbc.repository.config.EnableJdbcRepositories

@Configuration
@EnableJdbcRepositories(
    basePackages = ["ru.yandex.alice.paskill.dialogovo.scenarios.theremin"]
)
open class ThereminRepositoryConfig
