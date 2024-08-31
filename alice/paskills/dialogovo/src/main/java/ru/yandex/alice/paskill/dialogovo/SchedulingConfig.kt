package ru.yandex.alice.paskill.dialogovo

import org.springframework.boot.autoconfigure.condition.ConditionalOnProperty
import org.springframework.context.annotation.Configuration
import org.springframework.context.annotation.Profile
import org.springframework.scheduling.annotation.EnableScheduling

@ConditionalOnProperty(value = ["app.scheduling.enable"], havingValue = "true", matchIfMissing = true)
@Configuration
@EnableScheduling
@Profile("!ut")
open class SchedulingConfig
