package ru.yandex.alice.social.sharing

import org.springframework.boot.SpringApplication
import org.springframework.boot.WebApplicationType
import org.springframework.boot.autoconfigure.SpringBootApplication
import org.springframework.boot.autoconfigure.web.reactive.function.client.ClientHttpConnectorAutoConfiguration
import org.springframework.boot.autoconfigure.web.servlet.error.ErrorMvcAutoConfiguration
import org.springframework.scheduling.annotation.EnableScheduling

@SpringBootApplication(
    scanBasePackageClasses = [ Application::class ],
    exclude = [
        ClientHttpConnectorAutoConfiguration::class,
        ErrorMvcAutoConfiguration::class // remove /error page
    ]
)
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
