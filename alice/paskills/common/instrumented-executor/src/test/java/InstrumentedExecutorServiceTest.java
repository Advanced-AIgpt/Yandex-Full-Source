package ru.yandex.alice.paskills.common.executor;

import java.time.Duration;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.Executors;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.RejectedExecutionHandler;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;
import java.util.function.Supplier;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.Mockito;
import org.mockito.internal.verification.Times;

import ru.yandex.alice.paskills.common.solomon.utils.NamedSensorsRegistry;
import ru.yandex.monlib.metrics.histogram.Histograms;
import ru.yandex.monlib.metrics.primitives.Histogram;
import ru.yandex.monlib.metrics.primitives.Rate;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.when;

class InstrumentedExecutorServiceTest {

    private static final RejectedExecutionHandler DEFAULT_HANDLER = new ThreadPoolExecutor.AbortPolicy();

    private MetricRegistry metricRegistryMock;
    private Rate waitingsInvocations;
    private Rate invocationsInvocations;
    private Rate waitingsErrors;
    private Rate invocationsErrors;
    private Rate submitted;
    private Rate rejects;
    private Rate timeouts;

    private InstrumentedExecutorService createExecutorService(
            String name,
            int threads,
            int queueCapacity,
            ThreadFactory threadFactory
    ) {
        NamedSensorsRegistry namedSensorsRegistry = new NamedSensorsRegistry(
                metricRegistryMock.subRegistry("pool", name),
                "thread");
        BlockingQueue<Runnable> queue = queueCapacity > 0
                ? new LinkedBlockingQueue<>(queueCapacity)
                : new LinkedBlockingQueue<>();
        return new InstrumentedExecutorService(
                namedSensorsRegistry,
                new PoolConfig(
                        threads,
                        threads,
                        0L,
                        TimeUnit.MILLISECONDS),
                queue,
                threadFactory,
                DEFAULT_HANDLER,
                name
        );
    }

    private InstrumentedExecutorService createExecutorService(int poolSize, String name) {
        return createExecutorService(name, poolSize, 0, Executors.defaultThreadFactory());
    }

    private InstrumentedExecutorService createExecutorService(int poolSize, int queueCapacity, String name) {
        return createExecutorService(name, poolSize, queueCapacity, Executors.defaultThreadFactory());
    }

    @BeforeEach
    void before() {
        metricRegistryMock = Mockito.mock(MetricRegistry.class);

        MetricRegistry metricRegistry = new MetricRegistry();
        Rate rate = Mockito.spy(metricRegistry.rate("rate"));
        Histogram histogram = metricRegistry.histogramRate("histogram",
                Histograms.exponential(2, 1.5d, 10.3d / 1.5d));

        when(metricRegistryMock.rate(anyString())).thenReturn(rate);
        when(metricRegistryMock.histogramRate(anyString(), any(Supplier.class))).thenReturn(histogram);

        waitingsInvocations = Mockito.spy(metricRegistry.rate("thread.waitings.invocations"));
        when(metricRegistryMock.rate(eq("thread.waitings.invocations"))).thenReturn(waitingsInvocations);

        waitingsErrors = Mockito.spy(metricRegistry.rate("thread.waitings.errors"));
        when(metricRegistryMock.rate(eq("thread.waitings.errors"))).thenReturn(waitingsInvocations);

        invocationsInvocations = Mockito.spy(metricRegistry.rate("thread.invocations.invocations"));
        when(metricRegistryMock.rate(eq("thread.invocations.invocations"))).thenReturn(invocationsInvocations);

        invocationsErrors = Mockito.spy(metricRegistry.rate("thread.invocations.errors"));
        when(metricRegistryMock.rate(eq("thread.invocations.errors"))).thenReturn(invocationsErrors);

        submitted = Mockito.spy(metricRegistry.rate("thread.submitted"));
        when(metricRegistryMock.rate(eq("thread.submitted"))).thenReturn(submitted);

        rejects = Mockito.spy(metricRegistry.rate("thread.rejected"));
        when(metricRegistryMock.rate(eq("thread.rejected"))).thenReturn(rejects);

        timeouts = Mockito.spy(metricRegistry.rate("thread.timeouts"));
        when(metricRegistryMock.rate(eq("thread.timeouts"))).thenReturn(timeouts);

        when(metricRegistryMock.subRegistry(anyString(), anyString())).thenReturn(metricRegistryMock);
    }

    @Test
    void checkSupplyInvocationsMonitored1() {
        InstrumentedExecutorService executorService = createExecutorService(1, "executor");

        CompletableFuture<Integer> completableFuture1 = executorService.supplyAsyncInstrumented(() -> 1,
                Duration.ofSeconds(2));

        CompletableFuture<Void> completableFuture2 = executorService.supplyAsyncInstrumented(
                () -> {
                    throw new RuntimeException();
                }, Duration.ofSeconds(2));

        try {
            CompletableFuture.allOf(completableFuture1, completableFuture2).get(1, TimeUnit.SECONDS);
        } catch (Exception ex) {
        } finally {
            Mockito.verify(submitted, new Times(2)).inc();

            Mockito.verify(waitingsInvocations, new Times(2)).inc();
            Mockito.verify(waitingsErrors, never()).inc();

            Mockito.verify(invocationsInvocations, new Times(2)).inc();
            Mockito.verify(invocationsErrors, new Times(1)).inc();

            executorService.shutdownNow();
        }
    }

    @Test
    void checkSupplyInvocationsMonitoredOnLargePool() {
        InstrumentedExecutorService executorService = createExecutorService(10, "executor");

        CompletableFuture<Integer> completableFuture1 = executorService.supplyAsyncInstrumentedWithoutTimeout(() -> 1);

        CompletableFuture<Void> completableFuture2 = executorService.supplyAsyncInstrumentedWithoutTimeout(
                () -> {
                    throw new RuntimeException();
                });

        try {
            CompletableFuture.allOf(completableFuture1, completableFuture2).get(1, TimeUnit.SECONDS);
        } catch (Exception ex) {
        } finally {
            Mockito.verify(submitted, new Times(2)).inc();

            Mockito.verify(waitingsInvocations, new Times(2)).inc();
            Mockito.verify(waitingsErrors, never()).inc();

            Mockito.verify(invocationsInvocations, new Times(2)).inc();
            Mockito.verify(invocationsErrors, new Times(1)).inc();

            executorService.shutdownNow();
        }
    }

    @Test
    void checkSupplyInvocationsMonitored() {
        InstrumentedExecutorService executorService = createExecutorService(1, "executor");

        CompletableFuture<Integer> completableFuture1 = executorService.supplyAsyncInstrumented(() -> 1,
                Duration.ofSeconds(10));

        CompletableFuture<Integer> completableFuture2 = executorService.supplyAsyncInstrumented(
                () -> {
                    throw new RuntimeException();
                }, Duration.ofSeconds(10));

        try {
            CompletableFuture.allOf(completableFuture1, completableFuture2).get(1, TimeUnit.SECONDS);
        } catch (Exception ex) {
        } finally {
            Mockito.verify(submitted, new Times(2)).inc();

            Mockito.verify(waitingsInvocations, new Times(2)).inc();
            Mockito.verify(waitingsErrors, never()).inc();

            Mockito.verify(invocationsInvocations, new Times(2)).inc();
            Mockito.verify(invocationsErrors, new Times(1)).inc();

            executorService.shutdownNow();
        }
    }

    @Test
    void checkRunInvocationsMonitored() {
        InstrumentedExecutorService executorService = createExecutorService(1, "executor");

        CompletableFuture<Void> completableFuture1 = executorService.runAsyncInstrumented(() -> {
        }, Duration.ofSeconds(11));

        CompletableFuture<Void> completableFuture2 = executorService.runAsyncInstrumented(
                () -> {
                    throw new RuntimeException();
                }, Duration.ofSeconds(1));

        try {
            CompletableFuture.allOf(completableFuture1, completableFuture2).get(1, TimeUnit.SECONDS);
        } catch (Exception ex) {
        } finally {
            Mockito.verify(submitted, new Times(2)).inc();

            Mockito.verify(waitingsInvocations, new Times(2)).inc();
            Mockito.verify(waitingsErrors, never()).inc();

            Mockito.verify(invocationsInvocations, new Times(2)).inc();
            Mockito.verify(invocationsErrors, new Times(1)).inc();

            executorService.shutdownNow();
        }
    }

    @Test
    void checkTaskRejectsMonitored() {
        InstrumentedExecutorService executorService = createExecutorService(1, 1, "executor");

        CompletableFuture<Void> longTask = executorService.runAsyncInstrumented(() -> {
            try {
                Thread.sleep(2000);
            } catch (InterruptedException e) {

            }
        }, Duration.ofSeconds(10));

        CompletableFuture<Void> queuedTask = executorService.runAsyncInstrumented(
                () -> {
                    throw new RuntimeException();
                }, Duration.ofSeconds(10));

        try {
            CompletableFuture<Void> rejectedTask = executorService.runAsyncInstrumented(
                    () -> {
                        throw new RuntimeException();
                    }, Duration.ofSeconds(10));
        } catch (Exception ex) {

        }

        try {
            CompletableFuture.allOf(longTask, queuedTask).get(3, TimeUnit.SECONDS);
        } catch (Exception ex) {
        } finally {
            Mockito.verify(submitted, new Times(3)).inc();

            Mockito.verify(waitingsInvocations, new Times(2)).inc();
            Mockito.verify(waitingsErrors, never()).inc();

            Mockito.verify(invocationsInvocations, new Times(2)).inc();
            Mockito.verify(invocationsErrors, new Times(1)).inc();

            Mockito.verify(rejects, new Times(1)).inc();

            executorService.shutdownNow();
        }
    }

    @Test
    void checkTaskRejectsOnSupplyMonitored() {
        InstrumentedExecutorService executorService = createExecutorService(1, 1, "executor");

        CompletableFuture<Integer> longTask = executorService.supplyAsyncInstrumented(() -> {
            try {
                Thread.sleep(2000);
            } catch (InterruptedException e) {

            }

            return 1;
        }, Duration.ofSeconds(10));

        CompletableFuture<Void> queuedTask = executorService.supplyAsyncInstrumented(
                () -> {
                    throw new RuntimeException();
                }, Duration.ofSeconds(10));

        try {
            CompletableFuture<Void> rejectedTask = executorService.supplyAsyncInstrumented(
                    () -> {
                        throw new RuntimeException();
                    }, Duration.ofSeconds(10));
        } catch (Exception ex) {

        }

        try {
            CompletableFuture.allOf(longTask, queuedTask).get(3, TimeUnit.SECONDS);
        } catch (Exception ex) {
        } finally {
            Mockito.verify(submitted, new Times(3)).inc();

            Mockito.verify(waitingsInvocations, new Times(2)).inc();
            Mockito.verify(waitingsErrors, never()).inc();

            Mockito.verify(invocationsInvocations, new Times(2)).inc();
            Mockito.verify(invocationsErrors, new Times(1)).inc();

            Mockito.verify(rejects, new Times(1)).inc();

            executorService.shutdownNow();
        }
    }


    @Test
    void checkExecutorExternalHardTimeout() {
        InstrumentedExecutorService executorService = createExecutorService(1, 1, "executor");

        CompletableFuture<Void> veryLongTask = executorService.runAsyncInstrumented(() -> {
            try {
                Thread.sleep(10000);
            } catch (InterruptedException e) {

            }
        }, Duration.ofSeconds(30));

        long startTime = System.currentTimeMillis();
        CompletableFuture<Void> queuedTaskWithHardTimeout = executorService.runAsyncInstrumented(() -> {
        }, Duration.ofSeconds(1));

        try {
            queuedTaskWithHardTimeout.join();
        } catch (Exception ex) {
        }

        long awaitTime = System.currentTimeMillis() - startTime;

        assertTrue((awaitTime / 1000d) < 2.0);

        Mockito.verify(timeouts, new Times(1)).inc();

        executorService.shutdownNow();
    }

    @Test
    void checkDefaultValueExecutorExternalHardTimeout() {
        InstrumentedExecutorService executorService = createExecutorService(1, 1, "executor");

        CompletableFuture<Integer> veryLongTask = executorService.supplyAsyncInstrumentedWithoutTimeout(() -> {
            try {
                Thread.sleep(10000);
            } catch (InterruptedException e) {

            }
            return 1;
        });

        long startTime = System.currentTimeMillis();
        Integer defaultValue = 1;
        CompletableFuture<Integer> queuedTaskWithHardTimeout = executorService.supplyAsyncInstrumented(
                () -> 100,
                Duration.ofSeconds(1),
                () -> defaultValue);

        Integer value = queuedTaskWithHardTimeout.join();

        long awaitTime = System.currentTimeMillis() - startTime;

        assertTrue((awaitTime / 1000d) < 2.0);
        assertEquals(value, defaultValue);

        Mockito.verify(timeouts, new Times(1)).inc();

        executorService.shutdownNow();
    }

    @Test
    void checkSupplyExecutorExternalHardTimeout() {
        InstrumentedExecutorService executorService = createExecutorService(1, 1, "executor");

        CompletableFuture<Integer> veryLongTask = executorService.supplyAsyncInstrumented(() -> {
            try {
                Thread.sleep(10000);
            } catch (InterruptedException e) {

            }
            return 1;
        }, Duration.ofSeconds(10));

        long startTime = System.currentTimeMillis();
        CompletableFuture<Integer> queuedTaskWithHardTimeout = executorService.supplyAsyncInstrumented(
                () -> 100,
                Duration.ofSeconds(1));

        Exception exception = null;
        try {
            Integer value = queuedTaskWithHardTimeout.join();
        } catch (Exception ex) {
            exception = ex;
        }

        long awaitTime = System.currentTimeMillis() - startTime;

        System.out.println(awaitTime / 1000d);
        assertTrue((awaitTime / 1000d) < 2.0);
        assertTrue(exception != null);

        Mockito.verify(timeouts, new Times(1)).inc();

        executorService.shutdownNow();
    }


    @Test
    void checkRunExecutorFailProceed() {
        InstrumentedExecutorService executorService = createExecutorService(1, 1, "executor");

        CompletableFuture<Integer> veryLongTask = executorService.supplyAsyncInstrumented(() -> {
            try {
                Thread.sleep(10000);
            } catch (InterruptedException e) {

            }
            return 1;
        }, Duration.ofSeconds(10));

        CompletableFuture<Void> queuedTaskWithHardTimeout = executorService.runAsyncInstrumented(
                () -> {
                },
                Duration.ofSeconds(1));

        Exception exception = null;
        try {
            queuedTaskWithHardTimeout.join();
        } catch (Exception ex) {
            exception = ex;
        }

        assertTrue(exception != null);

        Mockito.verify(timeouts, new Times(1)).inc();

        executorService.shutdownNow();
    }
}
