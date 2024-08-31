package ru.yandex.alice.kronstadt.server

import com.google.common.base.Strings
import org.apache.logging.log4j.LogManager
import org.springframework.beans.factory.annotation.Autowired
import org.springframework.beans.factory.annotation.Value
import org.springframework.boot.autoconfigure.condition.ConditionalOnMissingBean
import org.springframework.boot.autoconfigure.condition.ConditionalOnProperty
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import org.springframework.context.annotation.Profile
import ru.yandex.alice.kronstadt.core.tvm.TvmDestinationRegistrar
import ru.yandex.alice.paskills.common.tvm.solomon.TvmClientWithSolomon
import ru.yandex.alice.paskills.common.tvm.spring.handler.TvmAuthorizationInterceptor
import ru.yandex.monlib.metrics.registry.MetricRegistry
import ru.yandex.passport.tvmauth.BlackboxEnv
import ru.yandex.passport.tvmauth.CheckedServiceTicket
import ru.yandex.passport.tvmauth.CheckedUserTicket
import ru.yandex.passport.tvmauth.ClientStatus
import ru.yandex.passport.tvmauth.NativeTvmClient
import ru.yandex.passport.tvmauth.TicketStatus
import ru.yandex.passport.tvmauth.TvmApiSettings
import ru.yandex.passport.tvmauth.TvmClient
import ru.yandex.passport.tvmauth.TvmToolSettings
import ru.yandex.passport.tvmauth.roles.Roles
import java.net.URI

@Configuration
internal open class TvmConfiguration {

    @Value("\${tvm.trustedDeveloperToken:}")
    private val trustedDeveloperToken: String? = null

    @Value("\${tvm.developerTrustedTokenEnabled:false}")
    private val developerTrustedTokenEnabled = false

    @Value("\${tvm.validateServiceTicket:true}")
    private val validateServiceTicket = false

    private val logger = LogManager.getLogger()

    @Profile("!ut")
    @ConditionalOnProperty(name = ["tvm.mode"], havingValue = "tvmtool", matchIfMissing = false)
    @Bean
    open fun tvmClientWithTvmTool(
        metricRegistry: MetricRegistry,
        @Value("\${tvm.url}") url: URI,
        @Value("\${tvm.token}") token: String
    ): TvmClient {
        logger.info("Creating TVM client for tvm daemon on {}", url)
        val settings = TvmToolSettings.create("dialogovo")
            .setPort(url.port)
            .setHostname(url.host)
            .setAuthToken(token)
        return TvmClientWithSolomon(NativeTvmClient(settings), metricRegistry)
    }

    @Profile("!ut")
    @ConditionalOnProperty(name = ["tvm.mode"], havingValue = "tvmapi", matchIfMissing = true)
    @Bean
    open fun tvmClientForApi(
        @Value("\${tvm.selfClientId}") clientId: Int,
        @Value("\${tvm.secret}") tvmSecret: String,
        @Value("\${tvm.blackboxEnv:PROD}") blackboxEnv: BlackboxEnv = BlackboxEnv.PROD,
        metricRegistry: MetricRegistry,
        @Autowired(required = false) tvmRegistrars: List<TvmDestinationRegistrar>?,
    ): TvmClient {

        val destinationAliases: Map<String, Int> = fillTvmAliasMap(tvmRegistrars ?: listOf())

        logger.info("Creating TVM client for tvmapi for ${clientId}")
        val settings = TvmApiSettings.create().setSelfTvmId(clientId).enableServiceTicketChecking()
            .enableUserTicketChecking(blackboxEnv)

        if (destinationAliases.isNotEmpty()) {
            logger.info("Configuring TVM destinations: {}", destinationAliases)
            settings.enableServiceTicketsFetchOptions(tvmSecret, destinationAliases)
        } else {
            logger.info("Skip TVM destination configuration. No TvmDestinationRegistrars provided in application context")
        }

        return TvmClientWithSolomon(NativeTvmClient(settings), metricRegistry)
    }

    private fun fillTvmAliasMap(tvmRegistrars: List<TvmDestinationRegistrar>): Map<String, Int> {
        val clientAliases = mutableMapOf<String, Int>()

        tvmRegistrars.forEach {

            mutableMapOf<String, Int>()
                .apply(it::register)
                .forEach { newEntry ->
                    if (newEntry.key in clientAliases && clientAliases[newEntry.key] != newEntry.value) {
                        throw RuntimeException(
                            "Unable to configure TvmApi. Alias ${newEntry.key} is " +
                                "already defined for ${clientAliases[newEntry.key]}"
                        )
                    }
                    clientAliases[newEntry.key] = newEntry.value
                }

        }

        return clientAliases
    }

    @ConditionalOnMissingBean(TvmClient::class)
    @Bean("tvmClient")
    @ConditionalOnProperty(name = ["tvm.mode"], havingValue = "disabled", matchIfMissing = false)
    open fun fakeTvmClient(): TvmClient {
        logger.info("Creating fake TVM client")
        return object : TvmClient {
            override fun getStatus(): ClientStatus {
                return ClientStatus(ClientStatus.Code.OK, "")
            }

            override fun getServiceTicketFor(alias: String): String {
                throw RuntimeException("Real TVM client is deeded but TVM was disabled")
            }

            override fun getServiceTicketFor(tvmId: Int): String {
                throw RuntimeException("Real TVM client is deeded but TVM was disabled")
            }

            override fun checkServiceTicket(ticketBody: String): CheckedServiceTicket {
                logger.warn("Fake TVM is used to check service ticket. Return fake ticker for dst_id=0")
                return CheckedServiceTicket(TicketStatus.OK, "", 1, 0L)
            }

            override fun checkUserTicket(ticketBody: String): CheckedUserTicket {
                logger.warn("Fake TVM is used to check user ticket. Return fake ticker for uid=0")
                return CheckedUserTicket(TicketStatus.OK, "", arrayOfNulls(0), 1L, longArrayOf(0L))
            }

            override fun checkUserTicket(ticketBody: String, overridedBbEnv: BlackboxEnv): CheckedUserTicket {
                return checkUserTicket(ticketBody)
            }

            override fun getRoles(): Roles {
                throw RuntimeException("Not implemented")
            }

            override fun close() {}
        }
    }

    @Bean
    open fun tvmAuthorizationInterceptor(tvmClient: TvmClient): TvmAuthorizationInterceptor {
        return TvmAuthorizationInterceptor(
            tvmClient, emptyMap(),
            trustedDeveloperToken.takeIf { developerTrustedTokenEnabled }?.let { Strings.emptyToNull(it) },
            developerTrustedTokenEnabled && trustedDeveloperToken.isNullOrEmpty(),
            validateServiceTicket
        )
    }
}
