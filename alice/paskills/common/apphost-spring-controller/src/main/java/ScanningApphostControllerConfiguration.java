package ru.yandex.alice.paskills.common.apphost.spring;

import java.util.List;
import java.util.Map;

import org.springframework.context.ApplicationContext;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.web.apphost.api.AppHostPathHandler;

@Configuration
public class ScanningApphostControllerConfiguration {

    private final ApplicationContext applicationContext;
    private final HandlerScanner scanner;

    public ScanningApphostControllerConfiguration(
            ApplicationContext applicationContext,
            HandlerScanner scanner
    ) {
        this.applicationContext = applicationContext;
        this.scanner = scanner;
    }

    @Bean
    public AdditionalHandlersSupplier scannedHandlersSupplier() {
        Map<String, Object> beans = applicationContext.getBeansWithAnnotation(ApphostController.class);

        List<? extends AppHostPathHandler> controllers = beans.values().stream()
                .flatMap(bean -> scanner.scanHandler(bean.getClass())
                        .stream()
                        .map(h -> new HandlerAdapter(bean, h))
                ).toList();
        return () -> controllers;
    }

}
