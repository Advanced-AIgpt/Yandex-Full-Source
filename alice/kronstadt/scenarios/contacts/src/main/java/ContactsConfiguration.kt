package ru.yandex.alice.kronstadt.scenarios.contacts

import org.springframework.boot.context.properties.ConfigurationPropertiesScan
import org.springframework.boot.context.properties.EnableConfigurationProperties
import org.springframework.context.annotation.Configuration
import org.springframework.context.annotation.PropertySource

@Configuration(proxyBeanMethods = false)
@EnableConfigurationProperties
@ConfigurationPropertiesScan(basePackageClasses = [ContactsScenario::class])
@PropertySource("classpath:/config/contacts-config.yaml")
@PropertySource("classpath:/config/contacts-config-\${spring.profiles.active}.yaml", ignoreResourceNotFound = true)
open class ContactsConfiguration
