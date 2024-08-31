package ru.yandex.quasar.billing.util;

import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.function.Function;
import java.util.function.Supplier;

import javax.annotation.Nullable;

import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.Data;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.apache.logging.log4j.ThreadContext;

import ru.yandex.quasar.billing.exception.AbstractHTTPException;
import ru.yandex.quasar.billing.exception.InternalErrorException;
import ru.yandex.quasar.billing.services.AuthorizationContext;

import static java.util.stream.Collectors.toList;
import static java.util.stream.Collectors.toMap;

public final class ParallelHelper {

    private static final Logger log = LogManager.getLogger();

    private final ExecutorService executorService;

    private final AuthorizationContext authorizationContext;

    public ParallelHelper(ExecutorService executorService, AuthorizationContext authorizationContext) {
        this.executorService = executorService;
        this.authorizationContext = authorizationContext;
    }

    /**
     * Applies {@param mapper} to each element of {@param source} collection in parallel preserving context
     * The order may not preserve
     */
    public <T, U> List<U> processParallel(Collection<T> source, Function<T, U> mapper) {
        if (source.size() > 1) {
            return parMap(source, mapper);
        } else {
            return source.stream().map(mapper).collect(toList());
        }
    }

    /**
     * Applies {@param mapper} to each element of {@param source} collection in parallel preserving context
     * The result or exception is stored in {@link TaskResult} object so the exceptions  may be handled manually.
     * The order may not preserve
     */
    public <T, U> List<TaskResult<U>> processParallelAsTasks(Collection<T> source, Function<T, U> mapper) {
        Function<T, TaskResult<U>> handlingMapper = it -> {
            try {
                return TaskResult.success(mapper.apply(it));
            } catch (RuntimeException e) {
                log.info("Exception while parallel processing", e);
                return TaskResult.error(e);
            }
        };
        return processParallel(source, handlingMapper);
    }

    public <T, U> Map<T, U> associateWith(Collection<T> source, Function<T, U> mapper) {
        if (source.size() > 1) {
            return parMap(source, item -> Map.entry(item, mapper.apply(item)))
                    .stream()
                    .collect(toMap(Map.Entry::getKey, Map.Entry::getValue));
        } else {
            return source.stream().collect(toMap(it -> it, mapper));
        }
    }

    public <U> CompletableFuture<U> async(Supplier<U> supplier) {
        AuthorizationContext.UserContext currentContext = authorizationContext.getUserContext().makeCopy();
        return CompletableFuture.supplyAsync(withContext(currentContext, supplier), executorService);
    }

    public CompletableFuture<Void> async(Runnable task) {
        AuthorizationContext.UserContext currentContext = authorizationContext.getUserContext().makeCopy();
        return CompletableFuture.runAsync(withContext(currentContext, task), executorService);
    }

    /**
     * Parallelly applies given `operation` to stream returning resulting stream or throwing an exception;
     *
     * @throws RuntimeException when some execution failed
     */
    private <T, U> List<U> parMap(Collection<T> source, Function<T, U> operation) {
        AuthorizationContext.UserContext currentUid = authorizationContext.getUserContext().makeCopy();

        List<CompletableFuture<U>> futures = source.stream()
                .map(
                        item -> CompletableFuture.supplyAsync(withContext(currentUid, () -> operation.apply(item)),
                                executorService
                        )
                )
                .collect(toList());

        try {
            return CompletableFuture
                    .allOf(futures.toArray(CompletableFuture[]::new))
                    .thenApply(aVoid -> futures.stream().map(CompletableFuture::join).collect(toList()))
                    .get();
        } catch (ExecutionException | InterruptedException e) {
            log.error("Parallel operation failed", e);
            if (e.getCause() instanceof AbstractHTTPException) {
                throw (AbstractHTTPException) e.getCause();
            } else {
                throw new InternalErrorException("Parallel operation failed", e);
            }
        }
    }

    private <T> Supplier<T> withContext(AuthorizationContext.UserContext executionContext, Supplier<T> operation) {
        return () -> {
            // in the async user context is independent as stored in ThreadLocal
            var userContext = authorizationContext.getUserContext();
            try {
                AuthorizationContext.UserContext copy = executionContext.makeCopy();
                authorizationContext.setUserContext(copy);
                ThreadContext.remove("uid");
                ThreadContext.remove("requestId");
                if (executionContext.getUid() != null) {
                    ThreadContext.put("uid", copy.getUid());
                }
                if (executionContext.getRequestId() != null) {
                    ThreadContext.put("requestId", copy.getRequestId());
                }
                return operation.get();
            } finally {
                authorizationContext.setUserContext(userContext);
                ThreadContext.remove("uid");
                ThreadContext.remove("requestId");
            }
        };
    }

    private Runnable withContext(AuthorizationContext.UserContext executionContext, Runnable operation) {
        return () -> {
            // in the async user context is independent as stored in ThreadLocal
            var userContext = authorizationContext.getUserContext();
            try {
                AuthorizationContext.UserContext copy = executionContext.makeCopy();
                authorizationContext.setUserContext(copy);
                ThreadContext.remove("uid");
                ThreadContext.remove("requestId");
                if (executionContext.getUid() != null) {
                    ThreadContext.put("uid", copy.getUid());
                }
                if (executionContext.getRequestId() != null) {
                    ThreadContext.put("requestId", copy.getRequestId());
                }
                operation.run();
            } finally {
                authorizationContext.setUserContext(userContext);
                ThreadContext.remove("uid");
                ThreadContext.remove("requestId");
            }
        };
    }

    @Data
    @AllArgsConstructor(access = AccessLevel.PRIVATE)
    public static class TaskResult<T> {
        @Nullable
        private final T result;
        @Nullable
        private final Throwable error;

        private static <T> TaskResult<T> success(T result) {
            return new TaskResult<>(result, null);
        }

        private static <T> TaskResult<T> error(Throwable error) {
            return new TaskResult<>(null, error);
        }

        public boolean isSuccessful() {
            return error == null;
        }
    }
}
