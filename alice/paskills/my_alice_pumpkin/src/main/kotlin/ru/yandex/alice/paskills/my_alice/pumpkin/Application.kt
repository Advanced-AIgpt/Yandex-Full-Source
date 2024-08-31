package ru.yandex.alice.paskills.my_alice.pumpkin

import org.springframework.boot.SpringApplication
import org.springframework.boot.WebApplicationType
import org.springframework.boot.autoconfigure.SpringBootApplication
import org.springframework.boot.autoconfigure.condition.ConditionalOnProperty
import org.springframework.boot.autoconfigure.web.reactive.function.client.ClientHttpConnectorAutoConfiguration
import org.springframework.boot.autoconfigure.web.servlet.error.ErrorMvcAutoConfiguration
import org.springframework.context.annotation.Configuration
import org.springframework.context.annotation.Profile
import org.springframework.scheduling.annotation.EnableScheduling

@SpringBootApplication(
    scanBasePackageClasses = [ Application::class ],
    exclude = [
        ClientHttpConnectorAutoConfiguration::class,
        ErrorMvcAutoConfiguration::class // remove /error page
    ])
@EnableScheduling
open class Application {
    companion object {
        @JvmStatic
        fun main(args: Array<String>) {
            val app = SpringApplication(Application::class.java)
            app.webApplicationType = WebApplicationType.SERVLET
            app.run();
        }
    }
}

@ConditionalOnProperty(value = ["app.scheduling.enable"], havingValue = "true", matchIfMissing = true)
@Configuration
@EnableScheduling
@Profile("!ut")
open class SchedulingConfig
