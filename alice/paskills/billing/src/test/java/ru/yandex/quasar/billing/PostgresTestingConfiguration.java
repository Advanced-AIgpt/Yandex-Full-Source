package ru.yandex.quasar.billing;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.sql.DriverManager;
import java.sql.SQLException;
import java.util.Objects;
import java.util.Properties;
import java.util.stream.Stream;

import javax.annotation.Nullable;
import javax.sql.DataSource;

import com.github.dockerjava.api.model.HostConfig;
import com.github.dockerjava.api.model.PortBinding;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.core.annotation.Order;
import org.springframework.core.io.support.PathMatchingResourcePatternResolver;
import org.springframework.jdbc.core.JdbcTemplate;
import org.springframework.jdbc.datasource.SimpleDriverDataSource;
import org.testcontainers.containers.PostgreSQLContainer;

import static ru.yandex.devtools.test.Paths.getSandboxResourcesRoot;
import static ru.yandex.quasar.billing.PostgresTestingConfiguration.PostgresInstanceFactory.PG_LOCAL_DATABASE;
import static ru.yandex.quasar.billing.PostgresTestingConfiguration.PostgresInstanceFactory.PG_LOCAL_PASSWORD;
import static ru.yandex.quasar.billing.PostgresTestingConfiguration.PostgresInstanceFactory.PG_LOCAL_PORT;
import static ru.yandex.quasar.billing.PostgresTestingConfiguration.PostgresInstanceFactory.PG_LOCAL_URL;
import static ru.yandex.quasar.billing.PostgresTestingConfiguration.PostgresInstanceFactory.PG_LOCAL_USER;


@Configuration
public class PostgresTestingConfiguration {

    private static final Logger logger = LogManager.getLogger("embedded-postgres");

    @Bean
    @Order(0)
    public PostgresInstance postgresInstance() {
        if (EmbeddedPostgresExecutionListener.instance == null) {
            var instance = PostgresInstanceFactory.create(null);
            EmbeddedPostgresExecutionListener.instance = instance;
            return instance;
        }
        return EmbeddedPostgresExecutionListener.instance;
    }

    @Bean
    public PostgresDataSourceConfig postgresDataSourceConfig(PostgresInstance instance) {
        return instance.config();
    }

    interface PostgresInstance {

        PostgresDataSourceConfig config();

        void cleanupInstance();
    }

    static class PostgresInstanceFactory {
        public static final String POSTGRES_VERSION = "10.6";
        public static final String PG_LOCAL_USER = "PG_LOCAL_USER";
        public static final String PG_LOCAL_PASSWORD = "PG_LOCAL_PASSWORD";
        public static final String PG_LOCAL_PORT = "PG_LOCAL_PORT";
        public static final String PG_LOCAL_DATABASE = "PG_LOCAL_DATABASE";
        public static final String PG_LOCAL_URL = "PG_LOCAL_URL";

        public static PostgresInstance create(@Nullable PreferredPostgresInstanceSettings settings) {
            if (getSandboxResourcesRoot() != null && "Linux".equals(System.getProperty("os.name"))) {
                logger.info("User Recipe db");
                return new PostgresSandboxInstance();
            } else {
                logger.info("User docker db");
                return new PostgresDockerInstance(settings);
            }
        }
    }

    record PreferredPostgresInstanceSettings(
            @Nullable String username,
            @Nullable String password,
            @Nullable Integer exposedPort,
            boolean logStatements
    ) {
    }

    public record PostgresDataSourceConfig(
            String username,
            String password,
            String url,
            int exposedPort
    ) {
    }

    static class PostgresSandboxInstance implements PostgresInstance {

        private final PostgresDataSourceConfig config;

        PostgresSandboxInstance() {

            var username = Objects.requireNonNull(System.getenv(PG_LOCAL_USER));
            var password = Objects.requireNonNull(System.getenv(PG_LOCAL_PASSWORD));
            int dbPort = Integer.parseInt(Objects.requireNonNull(System.getenv(PG_LOCAL_PORT)));
            var database = Objects.requireNonNull(System.getenv(PG_LOCAL_DATABASE));
            String url = "jdbc:postgresql://localhost:" + dbPort + "/" + database;
            this.config = new PostgresDataSourceConfig(username, password, url, dbPort);

            try {
                Properties props = new Properties();
                SimpleDriverDataSource ds = new SimpleDriverDataSource(
                        DriverManager.getDriver(config.url),
                        config.url,
                        config.username,
                        config.password
                );
                props.setProperty("ssl", "false");
                props.setProperty("sslmode", "allow");
                ds.setConnectionProperties(props);
                createDb(ds);
            } catch (Exception e) {
                throw new RuntimeException(e);
            }
        }

        private static void createDb(DataSource dataSource) throws IOException {
            byte[] ddlFileBytes = new PathMatchingResourcePatternResolver().getResource("create.sql")
                    .getInputStream()
                    .readAllBytes();
            String script = new String(ddlFileBytes, StandardCharsets.UTF_8);
            new JdbcTemplate(dataSource).execute(script);
        }

        @Override
        public PostgresDataSourceConfig config() {
            return config;
        }

        @Override
        public void cleanupInstance() {

        }
    }

    static class PostgresDockerInstance implements PostgresInstance {
        private final PostgreSQLContainer<?> container;
        private final PostgresDataSourceConfig config;

        PostgresDockerInstance(@Nullable PreferredPostgresInstanceSettings settings) {
            if (settings != null) {
                container = createContainer(
                        settings.username(),
                        settings.password(),
                        settings.exposedPort(),
                        Objects.requireNonNullElse(settings.logStatements, false)
                );
            } else {
                container = createContainer(
                        null,
                        null,
                        null/*exposedPort*/,
                        true
                );
            }
            container.withInitScript("create.sql");
            container.start();

            int exposedPort = container.getExposedPorts().stream().findFirst().orElseThrow();
            //int exposedPort = container.getFirstMappedPort();
            logger.info("Started container exporting port " + container.getExposedPorts());
            logger.info("Started container first mapped port " + exposedPort);
            logger.info("Started container binding ports " + container.getBinds());
            logger.info("Started container jdbcUrl " + container.getJdbcUrl());

            System.setProperty(PG_LOCAL_USER, container.getUsername());
            System.setProperty(PG_LOCAL_PASSWORD, container.getPassword());
            System.setProperty(PG_LOCAL_PORT, String.valueOf(exposedPort));
            System.setProperty(PG_LOCAL_DATABASE, container.getDatabaseName());
            System.setProperty(PG_LOCAL_URL, container.getJdbcUrl());

            config = new PostgresDataSourceConfig(
                    container.getUsername(),
                    container.getPassword(),
                    container.getJdbcUrl(),
                    exposedPort
            );

            try {
                Thread.sleep(500);
            } catch (InterruptedException e) {
                throw new RuntimeException(e);
            }

            var dataSource = new SimpleDriverDataSource(
                    container.getJdbcDriverInstance(),
                    container.getJdbcUrl(),
                    container.getUsername(),
                    container.getPassword()
            );

            var start = System.currentTimeMillis();
            boolean succeeded = false;
            while (System.currentTimeMillis() < start + 5000 && !succeeded) {
                try (var connection = dataSource.getConnection()) {
                    succeeded = connection.prepareStatement("select 1 as a").executeQuery().next();
                } catch (SQLException e) {
                    logger.info("Waiting for connection {}ms...", System.currentTimeMillis() - start);
                }

                if (!succeeded) {
                    try {
                        Thread.sleep(50);
                    } catch (InterruptedException e) {
                        throw new RuntimeException(e);
                    }
                }
            }

            if (!succeeded) {
                throw new RuntimeException("Can't obtain connection");
            }

        }

        private static PostgreSQLContainer<?> createContainer(
                @Nullable String username,
                @Nullable String password,
                @Nullable Integer exposedPort,
                boolean logStatements
        ) {
            var container = new PostgreSQLContainer<>("postgres:" + PostgresInstanceFactory.POSTGRES_VERSION);
            if (username != null) {
                container.withUsername(username);
            }
            if (password != null) {
                container.withPassword(password);
            }
            if (exposedPort != null) {
                container.withCreateContainerCmdModifier(cmd ->
                        cmd.withHostConfig(
                                HostConfig.newHostConfig()
                                        .withPortBindings(PortBinding.parse("5432:" + exposedPort))
                        )
                );
            }

            if (logStatements) {
                var commandParts = Stream.concat(Stream.of(container.getCommandParts()), Stream.of("-c",
                                "log_statement=all"))
                        .toArray(String[]::new);
                container.setCommandParts(commandParts);
            }

            return container;
        }

        @Override
        public PostgresDataSourceConfig config() {
            return config;
        }

        @Override
        public void cleanupInstance() {
            container.stop();
        }
    }
}
