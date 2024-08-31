package ru.yandex.alice.paskills.skills.bilenko

import com.fasterxml.jackson.databind.DeserializationFeature
import com.fasterxml.jackson.databind.PropertyNamingStrategy
import org.springframework.boot.SpringApplication
import org.springframework.boot.autoconfigure.SpringBootApplication
import org.springframework.boot.autoconfigure.web.reactive.function.client.ClientHttpConnectorAutoConfiguration
import org.springframework.boot.autoconfigure.web.servlet.error.ErrorMvcAutoConfiguration
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import org.springframework.http.converter.json.Jackson2ObjectMapperBuilder

@SpringBootApplication(scanBasePackageClasses = [Application::class],
    exclude = [ClientHttpConnectorAutoConfiguration::class, ErrorMvcAutoConfiguration::class // remove /error page
    ])
@Configuration
internal open class Application {
    //@Bean
    open fun jacksonMapperBuilder(): Jackson2ObjectMapperBuilder {
        return Jackson2ObjectMapperBuilder()
            .propertyNamingStrategy(PropertyNamingStrategy.SNAKE_CASE)
            //.featuresToDisable(DeserializationFeature.FAIL_ON_UNKNOWN_PROPERTIES)
    }

    companion object {
        @JvmStatic
        fun main(args: Array<String>) {
            val httpPort = System.getenv("HTTP_PORT")
            System.setProperty("server.port", httpPort ?: "8080")
            SpringApplication.run(Application::class.java)
        }
    }


}


