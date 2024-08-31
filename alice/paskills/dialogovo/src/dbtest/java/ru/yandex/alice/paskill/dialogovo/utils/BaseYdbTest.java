package ru.yandex.alice.paskill.dialogovo.utils;

import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.function.Consumer;
import java.util.function.Function;

import com.yandex.ydb.core.grpc.GrpcTransport;
import com.yandex.ydb.table.Session;
import com.yandex.ydb.table.SessionRetryContext;
import com.yandex.ydb.table.TableClient;
import com.yandex.ydb.table.rpc.grpc.GrpcTableRpc;
import com.yandex.ydb.table.transaction.TxControl;
import org.junit.jupiter.api.AfterAll;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.BeforeEach;
import org.springframework.core.io.support.PathMatchingResourcePatternResolver;

import ru.yandex.alice.paskill.dialogovo.ydb.YdbClient;

import static com.google.common.io.Files.asCharSource;

/**
 * Provide your YDB OAuth token via "ydbToken" environment variable
 * and database name via "database" variable
 */
public abstract class BaseYdbTest {

    protected static TableClient tableClient;
    protected static String ydbDatabase;
    protected static SessionRetryContext sessionRetryContext;
    protected static YdbClient ydbClient;

    @BeforeAll
    public static void createDatabase() throws IOException {
        ydbDatabase = System.getenv("YDB_DATABASE");
        String ydbEndpoint = System.getenv("YDB_ENDPOINT");

        GrpcTransport transport = GrpcTransport.forEndpoint(ydbEndpoint, ydbDatabase).build();

        int minPoolSize = 20;
        int maxPoolSize = 50;
        tableClient = TableClient.newClient(GrpcTableRpc.ownTransport(transport))
                .sessionPoolSize(minPoolSize, maxPoolSize)
                .queryCacheSize(maxPoolSize)
                .keepQueryText(true)
                .build();

        sessionRetryContext = SessionRetryContext.create(tableClient).build();
        ydbClient = new YdbClient(sessionRetryContext, List.of(), minPoolSize);

        File script = new PathMatchingResourcePatternResolver().getResource("create_tables.yql").getFile();
        String ddl = asCharSource(script, StandardCharsets.UTF_8).read();
        tableClient.createSession().join().expect("cannot create session")
                .executeSchemeQuery(
                        "PRAGMA TablePathPrefix(\"" + ydbDatabase + "\");\n" + ddl
                ).join().expect("can't create database");
    }

    @AfterAll
    public static void dropDatabase() throws IOException {
        File script = new PathMatchingResourcePatternResolver().getResource("drop_tables.yql").getFile();
        String ddl = asCharSource(script, StandardCharsets.UTF_8).read();
        tableClient.createSession().join().expect("cannot create session")
                .executeSchemeQuery(
                        "PRAGMA TablePathPrefix(\"" + ydbDatabase + "\");\n" + ddl
                ).join().expect("failed to drop database");
        tableClient.close();
    }

    @BeforeEach
    protected void setUp() throws Exception {
        clearAllTables();
    }

    @AfterEach
    protected void tearDown() throws Exception {
        clearAllTables();
    }

    protected void withSession(Consumer<Session> consumer) {
        Session session = null;

        try {
            session = tableClient.createSession().join().expect("session");
            consumer.accept(session);
        } finally {
            if (session != null) {
                session.close().join();
            }
        }
    }

    protected <R> R withSessionFun(Function<Session, R> func) {
        Session session = null;

        try {
            session = tableClient.createSession().join().expect("session");
            return func.apply(session);
        } finally {
            if (session != null) {
                session.close().join();
            }
        }
    }

    public void clearAllTables() throws IOException {
        File script = new PathMatchingResourcePatternResolver().getResource("delete_tables.yql").getFile();
        String ddl = asCharSource(script, StandardCharsets.UTF_8).read();
        TxControl txControl = TxControl.serializableRw().setCommitTx(true);
        tableClient.createSession().join().expect("cannot create session")
                .executeDataQuery(
                        "PRAGMA TablePathPrefix(\"" + ydbDatabase + "\");\n" + ddl,
                        txControl
                ).join().expect("can't clear database");
    }
}
