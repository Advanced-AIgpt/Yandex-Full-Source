package ru.yandex.alice.paskills.common.logging.protoseq;

import java.util.concurrent.atomic.AtomicLong;

import javax.annotation.Nullable;

import org.apache.logging.log4j.Marker;
import org.apache.logging.log4j.MarkerManager;
import org.apache.logging.log4j.ThreadContext;

/**
 * thread context helpers
 */
public class Setrace {

    private static final AtomicLong FRAME_ID = new AtomicLong();

    private final String serviceName;

    /**
     * One may create a marker using {@link org.apache.logging.log4j.MarkerManager}
     */
    public static final Marker SETRACE_TAG_MARKER_PARENT = MarkerManager.getMarker("SETRACE_TAG_PARENT");

    public Setrace(String serviceName) {
        this.serviceName = serviceName;
    }

    public void setupThreadContext(@Nullable String rtLogToken) {
        if (rtLogToken == null) {
            return;
        }
        String[] parts = rtLogToken.split("\\$");
        if (parts.length >= 3) {
            ThreadContext.put(ThreadContextKey.SERVICE_NAME.value(), serviceName);
            ThreadContext.put(ThreadContextKey.REQUEST_TIMESTAMP.value(), parts[0]);
            ThreadContext.put(ThreadContextKey.REQUEST_ID.value(), parts[1]);
            ThreadContext.put(ThreadContextKey.ACTIVATION_ID.value(), parts[2]);
            ThreadContext.put(ThreadContextKey.FRAME_ID.value(), Long.toString(FRAME_ID.incrementAndGet()));
        }
    }

    public void clearThreadContext() {
        ThreadContext.removeAll(ThreadContextKey.ALL_KEYS);
    }

}
