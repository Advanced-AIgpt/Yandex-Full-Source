package ru.yandex.alice.kronstadt.runner

import org.apache.logging.log4j.LogManager
import org.springframework.boot.SpringApplication
import org.springframework.boot.env.EnvironmentPostProcessor
import org.springframework.core.Ordered
import org.springframework.core.env.ConfigurableEnvironment

class VaultPropertySourceEnvironmentPostProcessor : EnvironmentPostProcessor, Ordered {

    override fun postProcessEnvironment(environment: ConfigurableEnvironment, application: SpringApplication) {
        VaultPropertySource.addToEnvironment(environment, logger)
    }

    override fun getOrder(): Int = ORDER

    companion object {
        /**
         * The default order of this post-processor.
         */
        private val logger = LogManager.getLogger()
        private val ORDER = Ordered.HIGHEST_PRECEDENCE + 1
    }
}
