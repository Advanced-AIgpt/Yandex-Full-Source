package ru.yandex.quasar.billing.util;

import java.time.Duration;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.function.Function;

import com.google.common.util.concurrent.ThreadFactoryBuilder;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.test.context.junit.jupiter.SpringJUnitConfig;

import ru.yandex.quasar.billing.services.AuthorizationContext;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTimeout;

@SpringJUnitConfig(classes = AuthorizationContext.class)
class ParallelHelperTest {

    public static final String UID = "999";
    private final int delay = 30;
    private ParallelHelper parallelHelper;
    @Autowired
    private AuthorizationContext authorizationContext;
    private ExecutorService executorService = Executors.newFixedThreadPool(4,
            new ThreadFactoryBuilder()
                    .setNameFormat("parhelper-test-%d")
                    .build()
    );

    @BeforeEach
    void setUp() {
        parallelHelper = new ParallelHelper(
                executorService,
                authorizationContext
        );

        authorizationContext.setCurrentUid(UID);
    }

    @Test
    void processParallel() {

        // given
        List<Integer> tasks = List.of(1, 2, 3, 4);

        // when
        List<String> result = assertTimeout(Duration.ofMillis(delay * tasks.size() - 1),
                () -> parallelHelper.processParallel(tasks, createTask()));

        // then
        // order must preserve
        assertEquals(List.of(UID + ".1", UID + ".2", UID + ".3", UID + ".4"), result);
    }

    @Test
    void AssociateWithTest() {
        // given
        List<Integer> tasks = List.of(1, 2, 3, 4);

        // when
        Map<Integer, String> result = assertTimeout(Duration.ofMillis(delay * tasks.size() - 1),
                () -> parallelHelper.associateWith(tasks, createTask()));

        // then
        assertEquals(Map.of(1, UID + ".1", 2, UID + ".2", 3, UID + ".3", 4, UID + ".4"), result);
    }

    @Test
    void associateWithSingletonTest() {
        // given
        List<Integer> tasks = List.of(1);

        // when
        Map<Integer, String> result = parallelHelper.associateWith(tasks, createTask());

        // then
        assertEquals(Map.of(1, UID + ".1"), result);
    }

    @Test
    void associateWithEmptyTest() {
        // given
        List<Integer> tasks = List.of();

        // when
        Map<Integer, String> result = parallelHelper.associateWith(tasks, createTask());

        // then
        assertEquals(Map.of(), result);
    }

    private Function<Integer, String> createTask() {
        return idx -> {
            try {
                Thread.sleep(delay);
                return authorizationContext.getUserContext().getUid() + "." + idx;
            } catch (InterruptedException e) {
                throw new RuntimeException(e);
            }
        };
    }
}
