package ru.yandex.alice.paskill.dialogovo;

import java.util.List;
import java.util.Optional;

import com.google.protobuf.Descriptors;
import com.google.protobuf.StringValue;
import org.eclipse.jetty.server.HttpConfiguration;
import org.eclipse.jetty.server.ServerConnector;
import org.eclipse.jetty.util.BlockingArrayQueue;
import org.springframework.beans.BeansException;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.beans.factory.config.BeanFactoryPostProcessor;
import org.springframework.beans.factory.config.ConfigurableListableBeanFactory;
import org.springframework.boot.context.properties.ConfigurationPropertiesScan;
import org.springframework.boot.context.properties.EnableConfigurationProperties;
import org.springframework.boot.web.embedded.jetty.JettyServletWebServerFactory;
import org.springframework.boot.web.servlet.server.ConfigurableServletWebServerFactory;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.PropertySource;

import ru.yandex.alice.kronstadt.core.utils.YamlPropertySourceFactory;
import ru.yandex.alice.memento.proto.UserConfigsProto;
import ru.yandex.alice.paskill.dialogovo.config.WebServerConfig;
import ru.yandex.alice.paskill.dialogovo.proto.ApplyArgsProto;
import ru.yandex.alice.paskill.dialogovo.proto.DialogovoStateProto;
import ru.yandex.alice.paskill.dialogovo.utils.jetty.InstrumentedHttpConnectionFactory;
import ru.yandex.alice.paskill.dialogovo.utils.jetty.InstrumentedQueuedThreadPool;
import ru.yandex.alice.paskill.dialogovo.utils.jetty.InstrumentedServerConnector;
import ru.yandex.alice.paskills.common.solomon.utils.NamedSensorsRegistry;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

@Configuration
@EnableConfigurationProperties
@ConfigurationPropertiesScan({
        "ru.yandex.alice.paskill.dialogovo.config",
        "ru.yandex.alice.paskill.dialogovo.scenarios"
})
@PropertySource(
        value = "classpath:/config/dialogovo-config-${spring.profiles.active}.yaml",
        factory = YamlPropertySourceFactory.class
)
class ApplicationMainConfiguration implements BeanFactoryPostProcessor {

    // register protobuf classes used in Any
    @Override
    public void postProcessBeanFactory(ConfigurableListableBeanFactory beanFactory) throws BeansException {
        List<Descriptors.Descriptor> descriptors = List.of(
                ApplyArgsProto.RelevantApplyArgs.getDescriptor(),
                DialogovoStateProto.State.getDescriptor(),
                UserConfigsProto.TNewsConfig.getDescriptor(),
                StringValue.getDescriptor()
        );
        descriptors.forEach(desc -> beanFactory.registerSingleton(desc.getFullName(), desc));
    }

    @Bean
    public ConfigurableServletWebServerFactory webServerFactory(
            WebServerConfig webServerConfig,
            @Qualifier("internalMetricRegistry") MetricRegistry metricRegistry
    ) {

        NamedSensorsRegistry webServerSensorsRegistry = new NamedSensorsRegistry(metricRegistry, "web-server");

        JettyServletWebServerFactory factory = new JettyServletWebServerFactory();

        BlockingArrayQueue<Runnable> queue = new BlockingArrayQueue<>(10, 10, webServerConfig.getQueueSize());
        InstrumentedQueuedThreadPool threadPool = new InstrumentedQueuedThreadPool(
                webServerConfig.getMinThreads(),
                webServerConfig.getMaxThreads(),
                60000,
                queue,
                webServerSensorsRegistry.sub("thread-pool"));

        factory.setThreadPool(threadPool);
        factory.setServerCustomizers(List.of(server -> {

            HttpConfiguration httpConfiguration = new HttpConfiguration();
            InstrumentedHttpConnectionFactory httpConnectionFactory = new InstrumentedHttpConnectionFactory(
                    httpConfiguration,
                    webServerSensorsRegistry.sub("connection-factory"));

            ServerConnector connector = new InstrumentedServerConnector(
                    server,
                    webServerSensorsRegistry.sub("server-connector"),
                    httpConnectionFactory);

            Optional.ofNullable(System.getProperty("server.port"))
                    .map(Integer::valueOf)
                    .ifPresent(connector::setPort);
            connector.setAcceptQueueSize(webServerConfig.getQueueSize());
            server.setConnectors(new ServerConnector[]{connector});
        }));

        return factory;
    }
}
