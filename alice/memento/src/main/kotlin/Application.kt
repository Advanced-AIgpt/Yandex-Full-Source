package ru.yandex.alice.memento

import org.springframework.boot.SpringApplication
import org.springframework.boot.autoconfigure.SpringBootApplication
import org.springframework.boot.autoconfigure.web.reactive.function.client.ClientHttpConnectorAutoConfiguration
import org.springframework.boot.autoconfigure.web.servlet.error.ErrorMvcAutoConfiguration
import java.util.Objects

@SpringBootApplication(
    scanBasePackages = ["ru.yandex.alice.memento", "ru.yandex.alice.paskills.common"],
    exclude = [
        ClientHttpConnectorAutoConfiguration::class,
        ErrorMvcAutoConfiguration::class // remove /error page
    ]
)
open class Application {

    companion object {
        @JvmStatic
        fun main(args: Array<String>) {
            val httpPort = System.getenv("HTTP_PORT")
            System.setProperty("server.port", Objects.requireNonNullElse(httpPort, "8080"))
            SpringApplication.run(Application::class.java)
        }
    }
}
