package ru.yandex.alice.paskills.my_alice;

import org.springframework.boot.web.embedded.jetty.JettyServletWebServerFactory;
import org.springframework.boot.web.servlet.server.ServletWebServerFactory;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

@Configuration
public class HttpConfiguration {

    @Bean
    public ServletWebServerFactory servletWebServerFactory() {
        JettyServletWebServerFactory factory = new JettyServletWebServerFactory();
        return factory;
    }

}
