package ru.yandex.quasar.billing.services.processing.trust;

import java.io.IOException;
import java.time.Instant;
import java.time.LocalDate;
import java.time.ZoneId;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.json.JsonTest;
import org.springframework.boot.test.json.JacksonTester;
import org.springframework.test.context.junit.jupiter.SpringExtension;

import static org.assertj.core.api.Assertions.assertThat;

@ExtendWith(SpringExtension.class)
@JsonTest
public class UpdateSubscriptionRequestTest {
    @Autowired
    private JacksonTester<UpdateSubscriptionRequest> tester;

    @Test
    public void test() throws IOException {

        Instant now = LocalDate.of(2018, 1, 1).atStartOfDay(ZoneId.of("UTC")).toInstant();
        assertThat(tester.write(new UpdateSubscriptionRequest(now)))
                .isEqualToJson("{\"finish_ts\":\"1514764800.000\"}");
    }
}
