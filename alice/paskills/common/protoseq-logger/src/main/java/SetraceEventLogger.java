package ru.yandex.alice.paskills.common.logging.protoseq;

import org.apache.logging.log4j.Level;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

public class SetraceEventLogger {

    private static final Logger logger = LogManager.getLogger();

    private final Level level;

    public SetraceEventLogger(Level level) {
        this.level = level;
    }

    public void log() {
        logger.log(level, level.name());
    }

}
