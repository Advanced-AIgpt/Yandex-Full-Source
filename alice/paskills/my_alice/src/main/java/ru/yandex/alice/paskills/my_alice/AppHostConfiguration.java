package ru.yandex.alice.paskills.my_alice;

import java.io.IOException;
import java.util.List;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.alice.paskills.my_alice.controller.PathHandler;
import ru.yandex.web.apphost.api.AppHostService;
import ru.yandex.web.apphost.api.AppHostServiceBuilder;

@Configuration
public class AppHostConfiguration {

    @Bean(destroyMethod = "stop")
    public AppHostService appHostService(
            @Value("${APP_APPHOST_PORT:10000}") int port,
            List<PathHandler> handlers
    ) throws IOException {

        AppHostServiceBuilder appHostServiceBuilder = AppHostServiceBuilder.forPort(port);

        for (var handler : handlers) {
            appHostServiceBuilder.withPathHandler(handler.getPath(), handler::handle);
        }

        AppHostService service = appHostServiceBuilder.build();

        service.start();

        return service;
    }
}
