package ru.yandex.alice.paskills.common.logging.protoseq;

import java.util.Map;

import org.apache.logging.log4j.ThreadContext;

/**
 * Utility class to temporarily add keys to log4j's ThreadContext map.
 */
public class ThreadContextOverride implements AutoCloseable {

    private final Map<String, String> previousContext;

    public ThreadContextOverride(Map<String, String> contextOverride) {
        this.previousContext = ThreadContext.getContext();
        ThreadContext.putAll(contextOverride);
    }

    @Override
    public void close() {
        ThreadContext.clearAll();
        ThreadContext.putAll(previousContext);
    }
}
