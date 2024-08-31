package ru.yandex.alice.kronstadt.server.apphost

import org.springframework.context.annotation.ComponentScan
import org.springframework.context.annotation.Configuration

@Configuration
@ComponentScan(basePackages = ["ru.yandex.alice.paskills.common.apphost.spring"])
open class ApphostControllerComponentScanConfiguration
