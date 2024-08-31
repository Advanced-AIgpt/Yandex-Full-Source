package ru.yandex.alice.memento.tvm;

import org.springframework.boot.test.context.TestConfiguration;
import org.springframework.context.annotation.Bean;

import ru.yandex.passport.tvmauth.TvmClient;

@TestConfiguration
public class TestTvmConfiguration {
    @Bean
    public TvmClient tvmClient() {
        return new UnitTestTvmClient();
    }
}
