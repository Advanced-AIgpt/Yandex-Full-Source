package ru.yandex.alice.paskills.common.apphost.spring;

import java.net.InetAddress;
import java.net.ServerSocket;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.concurrent.Executor;
import java.util.function.Function;
import java.util.stream.Collectors;

import javax.annotation.Nullable;
import javax.net.ServerSocketFactory;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.ApplicationContext;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.alice.paskills.common.apphost.spring.solomon.MetricServerInterceptor;
import ru.yandex.monlib.metrics.registry.MetricRegistry;
import ru.yandex.web.apphost.api.AppHostPathHandler;
import ru.yandex.web.apphost.api.AppHostService;
import ru.yandex.web.apphost.api.AppHostServiceBuilder;
import ru.yandex.web.apphost.api.grpc.AppHostTypedServant;
import ru.yandex.web.apphost.api.healthcheck.AppHostHealthCheck;
import ru.yandex.web.apphost.api.servant.AppHostGrpcProperties;

import static java.util.Objects.requireNonNullElse;

@Configuration
public class ApphostServiceConfiguration {

    @Value("${apphost.bossThreadsCount:}")
    @Nullable
    private Integer bossThreadsCount;

    @Value("${apphost.workerThreadsCount:}")
    @Nullable
    private Integer workerThreadsCount;

    @Value("${apphost.handlerThreadsCount:}")
    @Nullable
    private Integer handlerThreadsCount;

    @Value("${apphost.handlerQueueSize:}")
    @Nullable
    private Integer handlerQueueSize;

    @Value("${apphost.port:0}")
    private int port;

    @Value("${apphost.ammoGenerator.enable:false}")
    private boolean ammoGeneratorEnable;

    @Autowired(required = false)
    private Optional<AppHostHealthCheck> healthCheck;

    private static final Logger logger = LogManager.getLogger();


    private final ApplicationContext applicationContext;
    private final MetricRegistry metricRegistry;
    private final ObjectMapper objectMapper;

    public ApphostServiceConfiguration(
            ApplicationContext applicationContext,
            MetricRegistry metricRegistry,
            ObjectMapper objectMapper) {
        this.applicationContext = applicationContext;
        this.metricRegistry = metricRegistry;
        this.objectMapper = objectMapper;
    }

    @Bean
    public AppHostService appHostService(
            List<AppHostPathHandler> handlers,
            List<AppHostTypedServant> typedServants,
            List<AdditionalHandlersSupplier> additionalHandlersSuppliers,
            List<ApphostRequestInterceptor> interceptors,
            @Qualifier("apphostHandlerExecutor")
            @Autowired(required = false)
            @Nullable
            Executor executor
    ) {

        var props = new AppHostGrpcProperties();
        if (bossThreadsCount != null) {
            props = props.withBossThreadsCount(bossThreadsCount);
        }
        if (workerThreadsCount != null) {
            props = props.withWorkerThreadsCount(workerThreadsCount);
        }
        if (executor != null) {
            props.withExecutor(executor);
        } else {
            if (handlerThreadsCount != null) {
                props = props.withHandlerThreadsCount(handlerThreadsCount);
            }

            props = props.withHandlerQueueSize(
                    handlerQueueSize != null ? handlerQueueSize : props.getHandlerThreadsCount()
            );
        }

        props.addInterceptor(new MetricServerInterceptor(metricRegistry));
        if (ammoGeneratorEnable) {
            props.addInterceptor(new AmmoGenServerInterceptor(objectMapper));
        }

        int apphostPort;
        if (port != 0) {
            apphostPort = port;
        } else {
            apphostPort = findAvailablePort(10000);
        }
        var builder = AppHostServiceBuilder.forPort(apphostPort).useGrpc(props);

        List<AppHostPathHandler> allHandlers = new ArrayList<>(handlers);
        typedServants.forEach(servant -> allHandlers.addAll(servant.getHandlers()));
        additionalHandlersSuppliers.forEach(it -> allHandlers.addAll(it.supply()));

        Map<String, AppHostPathHandler> handlerMap = allHandlers.stream()
                .collect(Collectors.toMap(AppHostPathHandler::getPath,
                        Function.identity(),
                        (u, v) -> {
                            throw new RuntimeException("Duplicate Apphost handlers for path: " + u.getPath());
                        }
                ));

        var safeInterceptors = List.copyOf(interceptors);

        handlerMap.forEach((path, handler) -> {
            logger.info("Add apphost handler for path {}: {}",
                    path, requireNonNullElse(handler.getClass().getCanonicalName(), handler.getClass().getName()));

            builder.withPathHandler(path, new InterceptedHandler(path, handler, safeInterceptors));
        });

        healthCheck.ifPresent(builder::withHealthCheck);

        return builder.build();
    }

    private int findAvailablePort(int minPort) {
        for (int checkPort = minPort; checkPort < minPort + 1000; checkPort++) {
            if (isPortAvailable(checkPort)) {
                return checkPort;
            } else {
                checkPort++;
            }
        }
        throw new RuntimeException("Cant find available port for Apphost");
    }

    private boolean isPortAvailable(int port) {
        try {
            ServerSocket serverSocket = ServerSocketFactory.getDefault().createServerSocket(
                    port, 1, InetAddress.getByName("localhost"));
            serverSocket.close();
            return true;
        } catch (Exception ex) {
            return false;
        }
    }

    @Bean
    public AppHostServiceGracefulShutdownLifecycle gracefulShutdownLifecycle(AppHostService appHostService) {
        return new AppHostServiceGracefulShutdownLifecycle(appHostService);
    }

    @Bean
    public AppHostServiceStartStopLifecycle startStopLifecycle(AppHostService appHostService) {
        return new AppHostServiceStartStopLifecycle(applicationContext, appHostService);
    }


}
