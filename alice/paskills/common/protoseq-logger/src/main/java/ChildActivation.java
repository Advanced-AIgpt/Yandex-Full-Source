package ru.yandex.alice.paskills.common.logging.protoseq;

import java.util.List;
import java.util.UUID;
import java.util.concurrent.Callable;

import javax.annotation.Nullable;

import org.apache.logging.log4j.ThreadContext;

public class ChildActivation {

    private static final SetraceEventLogger CHILD_ACTIVATION_STARTED =
            new SetraceEventLogger(LogLevels.CHILD_ACTIVATION_STARTED);
    private static final SetraceEventLogger CHILD_ACTIVATION_FINISHED =
            new SetraceEventLogger(LogLevels.CHILD_ACTIVATION_FINISHED);

    private static final List<String> WRITABLE_KEYS = ThreadContextKey.stringValues(
            ThreadContextKey.CHILD_ACTIVATION_ID,
            ThreadContextKey.CHILD_ACTIVATION_DESCRIPTION,
            ThreadContextKey.CHILD_ACTIVATION_RESULT_OK);

    @Nullable
    private final String childActivationId;
    @Nullable
    private final String rtLogToken;
    private final String description;

    public ChildActivation(String description) {
        this.description = description;
        @Nullable String requestTimestamp = ThreadContext.get(ThreadContextKey.REQUEST_TIMESTAMP.value());
        @Nullable String requestId = ThreadContext.get(ThreadContextKey.REQUEST_ID.value());
        if (requestTimestamp != null && requestId != null) {
            this.childActivationId = UUID.randomUUID().toString();
            this.rtLogToken = requestTimestamp + "$" + requestId + "$" + childActivationId;
        } else {
            this.childActivationId = null;
            this.rtLogToken = null;
        }
    }

    @Nullable
    public String rtLogToken() {
        return rtLogToken;
    }

    public <T> T run(Callable<T> callable) throws Exception {
        if (childActivationId != null) {
            ThreadContext.put(ThreadContextKey.CHILD_ACTIVATION_ID.value(), childActivationId);
            ThreadContext.put(ThreadContextKey.CHILD_ACTIVATION_DESCRIPTION.value(), description);
            CHILD_ACTIVATION_STARTED.log();
            try {
                T result = callable.call();
                ThreadContext.put(ThreadContextKey.CHILD_ACTIVATION_RESULT_OK.value(), "true");
                return result;
            } catch (Exception e) {
                ThreadContext.put(ThreadContextKey.CHILD_ACTIVATION_RESULT_OK.value(), "false");
                throw e;
            } finally {
                CHILD_ACTIVATION_FINISHED.log();
                ThreadContext.removeAll(WRITABLE_KEYS);
            }
        } else {
            return callable.call();
        }
    }

}
