package ru.yandex.alice.paskills.my_alice;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.WebApplicationType;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.boot.autoconfigure.condition.ConditionalOnProperty;
import org.springframework.boot.web.servlet.support.SpringBootServletInitializer;
import org.springframework.context.annotation.ComponentScan;
import org.springframework.context.annotation.Configuration;

import ru.yandex.web.apphost.api.AppHostService;


@SuppressWarnings("HideUtilityClassConstructor")
@SpringBootApplication(scanBasePackageClasses = Application.class)
public class Application extends SpringBootServletInitializer {

    public static void main(String[] args) {
        var app = new SpringApplication(Application.class);
        // we use AppHostService to serve requests
        app.setWebApplicationType(WebApplicationType.SERVLET);
        app.run(args);
    }

    @ConditionalOnProperty(value = "app.scheduling.enable", havingValue = "true", matchIfMissing = true)
    @Configuration
    @ComponentScan(basePackageClasses = AppHostService.class)
    static class SchedulingConfig {
    }
}
