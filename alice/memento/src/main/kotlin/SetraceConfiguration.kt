package ru.yandex.alice.memento

import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import ru.yandex.alice.paskills.common.logging.protoseq.Setrace

@Configuration(proxyBeanMethods = false)
open class SetraceConfiguration {
    @Bean
    open fun setrace() = Setrace("memento")
}
