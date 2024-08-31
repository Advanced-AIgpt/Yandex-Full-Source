package ru.yandex.alice.memento.storage.ydb;

import java.nio.charset.StandardCharsets;
import java.util.concurrent.Executors;

import com.google.common.io.Files;
import com.yandex.ydb.core.grpc.GrpcTransport;
import com.yandex.ydb.table.SessionRetryContext;
import com.yandex.ydb.table.TableClient;
import com.yandex.ydb.table.rpc.grpc.GrpcTableRpc;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.junit.platform.launcher.TestExecutionListener;
import org.junit.platform.launcher.TestPlan;
import org.springframework.core.io.support.PathMatchingResourcePatternResolver;

public class YdbRecipeDatabaseTestExecutionListener implements TestExecutionListener {
    private static final Logger logger = LogManager.getLogger();

    @Override
    public void testPlanExecutionStarted(TestPlan testPlan) {
        String database = System.getenv("YDB_DATABASE");
        String endpoint = System.getenv("YDB_ENDPOINT");
        String token = System.getenv("YDB_TEST_TOKEN");

        logger.info("testPlanExecutionStarted. database: $database, endpoint: $endpoint");

        if ("local".equals(database) && endpoint != null && endpoint.startsWith("localhost") && token == null) {
            logger.info("Creating YDB schema from script");
            try (var transport = GrpcTransport.forEndpoint(endpoint, database).build()) {
                try (var tableClient = TableClient.newClient(GrpcTableRpc.ownTransport(transport))
                        .sessionPoolSize(5, 5)
                        .queryCacheSize(5)
                        .keepQueryText(true)
                        .build()) {


                    var context = SessionRetryContext.create(tableClient)
                            .executor(Executors.newCachedThreadPool())
                            .build();

                    var script =
                            new PathMatchingResourcePatternResolver()
                                    .getResource("classpath:create_schema.yql").getFile();
                    var ddl = Files.asCharSource(script, StandardCharsets.UTF_8).read();

                    logger.info("Script to execute:\n$ddl");


                    context.supplyStatus(session -> session.executeSchemeQuery(ddl))
                            .join()
                            .expect("can't create ydb database");

                    logger.info("YDB Schema created");
                }


            } catch (Exception e) {
                logger.error("can't create schema", e);
            }
        }
    }
}
