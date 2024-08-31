package ru.yandex.alice.paskill.dialogovo.scenarios.theremin

import com.fasterxml.jackson.databind.ObjectMapper
import com.fasterxml.jackson.module.kotlin.readValue
import org.springframework.boot.context.properties.ConfigurationPropertiesScan
import org.springframework.boot.context.properties.EnableConfigurationProperties
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import org.springframework.context.annotation.PropertySource
import org.springframework.core.io.support.PathMatchingResourcePatternResolver

@EnableConfigurationProperties
@ConfigurationPropertiesScan(basePackageClasses = [ThereminScenario::class])
@PropertySource("classpath:/config/theremin-config.yaml")
@PropertySource("classpath:/config/theremin-config-\${spring.profiles.active}.yaml", ignoreResourceNotFound = true)
@Configuration(proxyBeanMethods = false)
open class ThereminConfiguration {
    @Bean
    open fun thereminConfig(objectMapper: ObjectMapper): ThereminPacksConfig {
        return PathMatchingResourcePatternResolver()
            .getResource("theremin-packs-config.json")
            .inputStream
            .use { objectMapper.readValue(it) }
    }
}
