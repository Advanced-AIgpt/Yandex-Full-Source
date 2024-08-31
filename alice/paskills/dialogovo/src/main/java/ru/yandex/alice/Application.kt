package ru.yandex.alice

import org.springframework.boot.SpringApplication
import org.springframework.boot.autoconfigure.SpringBootApplication
import org.springframework.boot.autoconfigure.data.web.SpringDataWebAutoConfiguration
import org.springframework.boot.autoconfigure.gson.GsonAutoConfiguration
import org.springframework.boot.autoconfigure.web.reactive.function.client.ClientHttpConnectorAutoConfiguration
import org.springframework.boot.autoconfigure.web.servlet.error.ErrorMvcAutoConfiguration

@SpringBootApplication(
    scanBasePackages = ["ru.yandex.alice.paskill.dialogovo", "ru.yandex.alice.kronstadt"],
    exclude = [
        ClientHttpConnectorAutoConfiguration::class,
        GsonAutoConfiguration::class,
        SpringDataWebAutoConfiguration::class,
        // remove /error page
        ErrorMvcAutoConfiguration::class,
    ]
)
open class Application {
    companion object {
        @JvmStatic
        fun main(args: Array<String>) {
            System.setProperty("server.port", System.getenv("QLOUD_HTTP_PORT") ?: "8021")
            SpringApplication.run(Application::class.java)
        }
    }
}
