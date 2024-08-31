package ru.yandex.alice.paskill.dialogovo.utils.executor;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.TimeUnit;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

class ExecutorUtils {
    private static final Logger logger = LogManager.getLogger();

    private ExecutorUtils() {
        throw new UnsupportedOperationException();
    }
    public static void shutdownExecutor(ExecutorService executor, int timeout, TimeUnit timeUnit) {
        try {
            executor.shutdown();
            if (!executor.awaitTermination(timeout, timeUnit)) {
                logger.warn("Thread pool did not shut down gracefully within {} {}. Proceeding with forceful shutdown",
                        timeout, timeUnit);
                executor.shutdownNow();
            }
        } catch (InterruptedException ex) {
            logger.error("Error during executor service shutdown", ex);
            Thread.currentThread().interrupt();
        }
    }

}
