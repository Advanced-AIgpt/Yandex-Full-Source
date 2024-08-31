package ru.yandex.alice.kronstadt.server.apphost

import com.fasterxml.jackson.databind.ObjectMapper
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import ru.yandex.alice.kronstadt.core.RequestContext
import ru.yandex.alice.kronstadt.core.VersionProvider
import ru.yandex.alice.kronstadt.core.convert.ProtoUtil
import ru.yandex.alice.kronstadt.core.convert.response.ServerDirectiveConverter
import ru.yandex.alice.kronstadt.core.scenario.IScenario
import ru.yandex.alice.paskills.common.apphost.http.HttpRequestConverter
import ru.yandex.alice.paskills.common.apphost.spring.AdditionalHandlersSupplier
import ru.yandex.alice.paskills.common.apphost.spring.HandlerScanner

@Configuration
open class ApphostHandlersConfigurator(
    private val versionProvider: VersionProvider,
    private val protoUtil: ProtoUtil,
    private val requestContext: RequestContext,
    private val scanner: HandlerScanner,
    private val objectMapper: ObjectMapper,
    private val contextPopulator: ContextPopulator,
    private val serverDirectiveConverter: ServerDirectiveConverter,
) {

    @Bean
    open fun httpRequestConverter() = HttpRequestConverter(objectMapper)

    @Bean
    open fun scenariosAppHostCustomizer(scenarios: List<IScenario<*>>): AdditionalHandlersSupplier {
        val handlerAdapters = scenarios.flatMap { scenario ->
            val adapter = ApphostScenarioAdapter(
                scenario,
                versionProvider,
                protoUtil,
                requestContext,
                contextPopulator,
                scanner,
                httpRequestConverter(),
            )
            val grpcAdapters =
                scenario.grpcHandlers.flatMap { handler ->
                    RpcHandlerAdapter(
                        scenarioMeta = scenario.scenarioMeta,
                        handler = handler,
                        httpConverter = httpRequestConverter(),
                        requestContext = requestContext,
                        contextPopulator = contextPopulator,
                        serverDirectiveConverter = serverDirectiveConverter,
                        objectMapper = objectMapper
                    ).getHandlerAdapters()
                }
            return@flatMap adapter.getHandlerAdapters() + grpcAdapters
        }
        return AdditionalHandlersSupplier { handlerAdapters }
    }
}



