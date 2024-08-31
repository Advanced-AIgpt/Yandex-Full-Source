package ru.yandex.quasar.billing;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.boot.autoconfigure.condition.ConditionalOnProperty;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.scheduling.annotation.EnableScheduling;
import org.springframework.scheduling.concurrent.ThreadPoolTaskScheduler;
import org.springframework.transaction.annotation.EnableTransactionManagement;

@SpringBootApplication(
        scanBasePackages = {"ru.yandex.quasar.billing", "ru.yandex.alice.library.routingdatasource"}
)
@EnableTransactionManagement(proxyTargetClass = true)
public class BillingServer {

    public static void main(String[] args) {
        String qloudHttpPort = System.getenv("QLOUD_HTTP_PORT");
        if (qloudHttpPort != null) {
            System.setProperty("server.port", qloudHttpPort);
        }

        // all endpoints will be available on root like /unistat
        System.setProperty("management.port", "80");
        System.setProperty("management.endpoints.web.base-path", "/");
        System.setProperty("management.endpoints.web.exposure.include", "health,unistat,healthcheck");

        SpringApplication.run(BillingServer.class);
    }

    @Bean
    public ThreadPoolTaskScheduler taskScheduler() {
        ThreadPoolTaskScheduler result = new ThreadPoolTaskScheduler();
        result.setPoolSize(5);
        return result;
    }

    @ConditionalOnProperty(
            value = "app.scheduling.enable", havingValue = "true", matchIfMissing = true
    )
    @Configuration
    @EnableScheduling
    public static class SchedulingConfiguration {
    }
}
