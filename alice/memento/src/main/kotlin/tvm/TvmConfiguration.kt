package ru.yandex.alice.memento.tvm

import org.springframework.beans.factory.annotation.Value
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import org.springframework.context.annotation.Profile
import ru.yandex.alice.paskills.common.tvm.solomon.TvmClientWithSolomon
import ru.yandex.monlib.metrics.registry.MetricRegistry
import ru.yandex.passport.tvmauth.NativeTvmClient
import ru.yandex.passport.tvmauth.TvmClient
import ru.yandex.passport.tvmauth.TvmToolSettings

@Configuration
open class TvmConfiguration {
    @Value("\${tvm.hostname:localhost}")
    private val hostname: String = ""

    @Value("\${tvm.port}")
    private val port = 0

    @Value("\${tvm.authToken}")
    private val token: String = ""

    open fun tvmToolSettings(): TvmToolSettings {
        return TvmToolSettings.create("memento")
            .setPort(port)
            .setHostname(hostname)
            .setAuthToken(token)
    }

    @Profile("!ut")
    @Bean
    open fun tvmClient(registry: MetricRegistry?): TvmClient {
        return TvmClientWithSolomon(NativeTvmClient(tvmToolSettings()), registry)
    }
}
