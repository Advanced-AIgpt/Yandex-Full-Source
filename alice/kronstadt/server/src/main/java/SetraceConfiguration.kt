package ru.yandex.alice.kronstadt.server

import org.springframework.beans.factory.annotation.Value
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import ru.yandex.alice.paskills.common.logging.protoseq.Setrace

@Configuration
open class SetraceConfiguration {
    @Bean
    open fun setrace(@Value("\${setrace.service_name:kronstadt}") serviceName: String) = Setrace(serviceName)
}
