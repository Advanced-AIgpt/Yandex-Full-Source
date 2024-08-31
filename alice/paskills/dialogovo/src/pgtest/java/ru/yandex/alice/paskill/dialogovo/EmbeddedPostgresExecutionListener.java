package ru.yandex.alice.paskill.dialogovo;

import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.HashSet;
import java.util.Optional;

import javax.sql.DataSource;

import de.flapdoodle.embed.process.config.IRuntimeConfig;
import de.flapdoodle.embed.process.config.io.ProcessOutput;
import de.flapdoodle.embed.process.io.Slf4jLevel;
import de.flapdoodle.embed.process.io.Slf4jStreamProcessor;
import de.flapdoodle.embed.process.io.directories.FixedPath;
import de.flapdoodle.embed.process.io.progress.Slf4jProgressListener;
import de.flapdoodle.embed.process.runtime.Network;
import de.flapdoodle.embed.process.store.PostgresArtifactStoreBuilder;
import org.junit.jupiter.api.extension.ExtendWith;
import org.junit.platform.engine.support.descriptor.ClassSource;
import org.junit.platform.launcher.TestExecutionListener;
import org.junit.platform.launcher.TestIdentifier;
import org.junit.platform.launcher.TestPlan;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.core.io.support.PathMatchingResourcePatternResolver;
import org.springframework.jdbc.BadSqlGrammarException;
import org.springframework.jdbc.core.JdbcTemplate;
import org.springframework.jdbc.datasource.DriverManagerDataSource;

import ru.yandex.qatools.embed.postgresql.Command;
import ru.yandex.qatools.embed.postgresql.PostgresExecutable;
import ru.yandex.qatools.embed.postgresql.PostgresProcess;
import ru.yandex.qatools.embed.postgresql.PostgresStarter;
import ru.yandex.qatools.embed.postgresql.config.AbstractPostgresConfig;
import ru.yandex.qatools.embed.postgresql.config.PostgresConfig;
import ru.yandex.qatools.embed.postgresql.config.PostgresDownloadConfigBuilder;
import ru.yandex.qatools.embed.postgresql.config.RuntimeConfigBuilder;
import ru.yandex.qatools.embed.postgresql.distribution.Version;
import ru.yandex.qatools.embed.postgresql.ext.LogWatchStreamProcessor;

import static java.util.Objects.requireNonNullElse;
import static ru.yandex.devtools.test.Paths.getSandboxResourcesRoot;

public class EmbeddedPostgresExecutionListener implements TestExecutionListener {

    private static final String USER = "bob";
    private static final String PASS = "ninja";
    private static final Logger logger = LoggerFactory.getLogger("embedded-postgres");
    private static int port;
    private static String databaseName;
    private static volatile DataSource dataSource;
    private PostgresProcess process;

    public EmbeddedPostgresExecutionListener() throws IOException {
        port = Network.getFreeServerPort();
    }

    public static DataSource createDataSource() {
        return dataSource != null ? dataSource : new DriverManagerDataSource("jdbc:postgresql://localhost:" + port +
                "/" + databaseName, USER, PASS);
    }

    @Override
    public void testPlanExecutionStarted(TestPlan testPlan) {
        // check if there are any TestClasses with EmbeddedPostgresExtension in the TestPlan
        boolean postgresRequired = testPlan.getRoots().stream()
                .map(testPlan::getChildren)
                .flatMap(Collection::stream)
                .map(TestIdentifier::getSource)
                .flatMap(Optional::stream)
                .filter(it -> it instanceof ClassSource)
                .map(ClassSource.class::cast)
                .map(ClassSource::getJavaClass)
                .map(clazz -> clazz.getAnnotationsByType(ExtendWith.class))
                .flatMap(Arrays::stream)
                .map(ExtendWith::value)
                .flatMap(Arrays::stream)
                .anyMatch(EmbeddedPostgresExtension.class::equals);


        if (postgresRequired) {
            try {
                PostgresConfig postgresConfig = new PostgresConfig(
                        Version.V10_6,
                        new AbstractPostgresConfig.Net("localhost", /*Network.getFreeServerPort()*/port),
                        new AbstractPostgresConfig.Storage("pgdb-" + System.currentTimeMillis()),
                        new AbstractPostgresConfig.Timeout(60_000),
                        new AbstractPostgresConfig.Credentials(USER, PASS),
                        Command.PgCtl
                );

                PostgresStarter<PostgresExecutable, PostgresProcess> runtime =
                        PostgresStarter.getInstance(
                                marketAutoorderRuntimeConfig(
                                        new File(requireNonNullElse(getSandboxResourcesRoot(), ""))
                                                .getAbsolutePath())
                        );

                PostgresExecutable exec = runtime.prepare(postgresConfig);
                process = exec.start();

                port = process.getConfig().net().port();
                databaseName = process.getConfig().storage().dbName();
                dataSource = createDataSource();

                createDb(dataSource);

                System.setProperty("PG_MULTI_HOST", "localhost:" + port);
                System.setProperty("PG_DATABASENAME", databaseName);
                System.setProperty("PG_USER", USER);
                System.setProperty("PG_PASSWORD", PASS);

            } catch (IOException e) {
                throw new RuntimeException(e);
            } catch (BadSqlGrammarException ex) {
                logger.error("SQL grammar exception: " + ex.getSQLException());
            }
        }
    }

    // see https://a.yandex-team.ru/arc/trunk/arcadia/market/mbi/autoorder/src/test
    private IRuntimeConfig marketAutoorderRuntimeConfig(String path) {
        Command cmd = Command.PgCtl;
        LogWatchStreamProcessor logWatch = new LogWatchStreamProcessor(
                "started", new HashSet<>(Collections.singletonList("failed")),
                new Slf4jStreamProcessor(logger, Slf4jLevel.TRACE));
        return new RuntimeConfigBuilder()
                .defaults(cmd)
                .artifactStore(new PostgresArtifactStoreBuilder()
                        .defaults(cmd)
                        .download(new PostgresDownloadConfigBuilder()
                                .defaultsForCommand(cmd)
                                .progressListener(new Slf4jProgressListener(logger))
                                .artifactStorePath(new FixedPath(path))
                                .build()))
                .processOutput(new ProcessOutput(logWatch, logWatch, logWatch)).build();
    }

    @Override
    public void testPlanExecutionFinished(TestPlan testPlan) {
        if (process != null) {
            System.out.println("STOPPING EMBEDDED DB");
            process.stop();
            process = null;
        }
        dataSource = null;
    }

    private void createDb(DataSource dataSource) throws IOException {
        File script = new PathMatchingResourcePatternResolver().getResource("create.sql").getFile();
        String ddl = com.google.common.io.Files.asCharSource(script, StandardCharsets.UTF_8).read();
        System.out.println("Creating database");
        new JdbcTemplate(dataSource).execute(ddl);
        System.out.println("Database created");
    }
}
