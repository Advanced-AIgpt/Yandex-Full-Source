package ru.yandex.alice.paskills.common.logging.protoseq;

import org.apache.logging.log4j.Level;

public final class LogLevels {

    public static final Level ACTIVATION_STARTED = Level.forName("ACTIVATION_STARTED", Level.INFO.intLevel());
    public static final Level ACTIVATION_FINISHED = Level.forName("ACTIVATION_FINISHED", Level.INFO.intLevel());
    public static final Level CHILD_ACTIVATION_STARTED =
            Level.forName("CHILD_ACTIVATION_STARTED", Level.INFO.intLevel());
    public static final Level CHILD_ACTIVATION_FINISHED =
            Level.forName("CHILD_ACTIVATION_FINISHED", Level.INFO.intLevel());

    private LogLevels() {
        throw new UnsupportedOperationException();
    }
}
