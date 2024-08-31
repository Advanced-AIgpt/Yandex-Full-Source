package ru.yandex.quasar.billing.services;

import java.time.Instant;
import java.util.ArrayList;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.apache.logging.log4j.Level;
import org.apache.logging.log4j.status.StatusData;
import org.apache.logging.log4j.status.StatusListener;
import org.apache.logging.log4j.status.StatusLogger;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.json.JsonTest;

import static org.junit.jupiter.api.Assertions.assertTrue;


@JsonTest
class YTLoggingServiceTest {

    @Autowired
    private ObjectMapper objectMapper;

    /**
     * Jackson 2.12.* somehow breaks log4j2 JsonLayout for java.time type
     * This test validates that YTRequestLogItem with requestTime Instant field serializes correctly
     */
    @Test
    void logRequestData() {

        var service = new YTLoggingService("localhost", "localhost", objectMapper);

        YTRequestLogItem logItem = new YTRequestLogItem();
        logItem.setHost("yandex.ru");
        logItem.setRequestId("req_id");
        logItem.setRequestTime(Instant.now());
        var statusLogger = StatusLogger.getLogger();
        var dataList = new ArrayList<StatusData>();

        statusLogger.registerListener(new StatusListener() {
            @Override
            public void log(StatusData data) {
                dataList.add(data);
            }

            @Override
            public Level getStatusLevel() {
                return Level.ERROR;
            }

            @Override
            public void close() {

            }
        });

        service.logRequestData("msg", logItem, null);

        assertTrue(dataList.isEmpty());

    }
}
