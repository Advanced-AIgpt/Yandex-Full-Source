package ru.yandex.alice.paskill.dialogovo.config

import org.springframework.context.annotation.ComponentScan
import org.springframework.context.annotation.Configuration

@Configuration
@ComponentScan(basePackages = ["ru.yandex.alice.paskills.common.resttemplate.factory"])
open class RestTemplateFactoryConfiguration