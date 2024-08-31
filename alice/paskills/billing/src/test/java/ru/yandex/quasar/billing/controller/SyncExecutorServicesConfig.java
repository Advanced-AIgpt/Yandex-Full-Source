package ru.yandex.quasar.billing.controller;

import java.util.concurrent.ExecutorService;

import org.springframework.boot.test.context.TestConfiguration;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Primary;

import ru.yandex.quasar.billing.util.SynchronousExecutorService;

@TestConfiguration()
public class SyncExecutorServicesConfig {

    @Primary
    @Bean(destroyMethod = "shutdownNow")
    public ExecutorService skillServiceExecutorService() {
        return new SynchronousExecutorService();
    }

    @Primary
    @Bean(destroyMethod = "shutdownNow")
    public ExecutorService contentExecutorService() {
        return new SynchronousExecutorService();
    }
}
