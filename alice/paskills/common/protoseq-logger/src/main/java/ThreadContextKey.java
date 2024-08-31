package ru.yandex.alice.paskills.common.logging.protoseq;

import java.util.Arrays;
import java.util.List;
import java.util.stream.Collectors;

enum ThreadContextKey {

    REQUEST_ID("setrace_request_id"),
    REQUEST_TIMESTAMP("setrace_request_timestamp"),
    ACTIVATION_ID("setrace_activation_id"),
    SERVICE_NAME("setrace_service_name"),
    FRAME_ID("setrace_frame_id"),
    CHILD_ACTIVATION_ID("setrace_child_activation_id"),
    CHILD_ACTIVATION_DESCRIPTION("setrace_child_activation_description"),
    CHILD_ACTIVATION_RESULT_OK("setrace_child_activation_result_ok");

    static final List<String> ALL_KEYS = stringValues(ThreadContextKey.values());

    private final String value;

    ThreadContextKey(String value) {
        this.value = value;
    }

    public String value() {
        return value;
    }

    public static List<String> stringValues(ThreadContextKey... keys) {
        return Arrays.stream(keys)
                .map(ThreadContextKey::value)
                .collect(Collectors.toList());
    }
}
