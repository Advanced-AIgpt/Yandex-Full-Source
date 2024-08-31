package ru.yandex.alice.paskill.dialogovo.service.logging;

import java.time.Duration;
import java.time.Instant;
import java.util.Collections;
import java.util.concurrent.CompletableFuture;
import java.util.stream.IntStream;

import javax.annotation.Nonnull;

import com.google.common.base.Stopwatch;
import com.yandex.ydb.table.transaction.TxControl;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Disabled;
import org.junit.jupiter.api.Test;

import ru.yandex.alice.kronstadt.core.RequestContext;
import ru.yandex.alice.paskill.dialogovo.midleware.DialogovoRequestContext;
import ru.yandex.alice.paskill.dialogovo.utils.BaseYdbTest;
import ru.yandex.alice.paskill.dialogovo.utils.executor.DialogovoInstrumentedExecutorService;
import ru.yandex.alice.paskill.dialogovo.utils.executor.ExecutorsFactory;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

import static java.util.stream.Collectors.toList;
import static org.junit.jupiter.api.Assertions.assertEquals;

class YdbSkillRequestPersistentTest extends BaseYdbTest {

    private YdbSkillRequestPersistent skillLogDao;
    private DialogovoInstrumentedExecutorService executorService;
    private MetricRegistry metricRegistry;

    @BeforeEach
    @Override
    public void setUp() throws Exception {
        super.setUp();
        ExecutorsFactory executorsFactory = new ExecutorsFactory(new MetricRegistry(), new RequestContext(),
                new DialogovoRequestContext());
        executorService = executorsFactory.fixedThreadPool(60, 1000000, "test");
        metricRegistry = new MetricRegistry();
        skillLogDao = new YdbSkillRequestPersistent(ydbClient, executorService, metricRegistry);
    }

    @AfterEach
    @Override
    public void tearDown() throws Exception {
        executorService.shutdownNow();
        super.tearDown();
    }

    @Test
    void testAddValueToUserState() {
        skillLogDao.prepareQueries();

        var record = new LogRecord("skill-id", "session-id", 0L, "event-id", Instant.now(), "{}", "{}",
                LogRecord.Status.OK, Collections.emptyList(), 50, "request-id", "device-id", "uuid");

        long before = getSkillRequestLogRecordsCount();

        var future = skillLogDao.save(record);

        future.join();

        Long actual = getSkillRequestLogRecordsCount();

        assertEquals(before + 1L, actual);
    }

    @Test
    void testAddNullableNull() {
        skillLogDao.prepareQueries();

        var record = new LogRecord("skill-id", "session-id", 0L, null, Instant.now(), null, null,
                LogRecord.Status.HTTP_ERROR_400, Collections.emptyList(), 50, null, null, null);

        long before = getSkillRequestLogRecordsCount();

        var future = skillLogDao.save(record);

        future.join();

        Long actual = getSkillRequestLogRecordsCount();

        assertEquals(before + 1L, actual);
    }

    // perf test. remove table clearing routine before executing to populate table with data
    @Disabled
    @Test
    void addManyRecords() {
        skillLogDao.prepareQueries();

        var record = new LogRecord("skill-id2", "session-id", 0L, "event-id", Instant.now(), "{}", "{}",
                LogRecord.Status.OK, Collections.emptyList(), 50, "request-id", "device-id", "uuid");

        var s = Stopwatch.createStarted();
        var sw = IntStream.range(0, 12000)
                .mapToObj(i -> {
                    try {
                        Thread.sleep(1);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                        throw new RuntimeException(e);
                    }
                    Stopwatch v = Stopwatch.createStarted();
                    return skillLogDao.save(record)
                            .handle((r, e) -> {
                                if (e != null) {
                                    System.out.println(Thread.currentThread().getName() + ": " + e.getMessage());
                                }
                                return v.stop().elapsed();
                            });
                })
                .collect(toList())
                .stream()
                .map(CompletableFuture::join)
                .map(Duration::toMillis)
                .sorted()
                .collect(toList());

        System.out.println(s.elapsed().toMillis());
        System.out.println(sw);
        System.out.println(sw.get(sw.size() - 1));
        System.out.println(getSkillRequestLogRecordsCount());
    }

    @Nonnull
    private Long getSkillRequestLogRecordsCount() {
        return withSessionFun(session -> {
                    var resultSet = session.executeDataQuery("select count(*) from skill_request_log",
                            TxControl.serializableRw())
                            .join()
                            .expect("")
                            .getResultSet(0);
                    resultSet.next();
                    return resultSet
                            .getColumn(0)
                            .getValue()
                            .asData()
                            .getUint64();
                }
        );
    }
}
