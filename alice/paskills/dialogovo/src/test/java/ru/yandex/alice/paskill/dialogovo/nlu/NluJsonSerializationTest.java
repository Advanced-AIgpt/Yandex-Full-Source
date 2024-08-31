package ru.yandex.alice.paskill.dialogovo.nlu;

import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.Map;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.apache.commons.io.IOUtils;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.json.JsonTest;
import org.springframework.core.io.Resource;
import org.springframework.core.io.support.PathMatchingResourcePatternResolver;

import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.DatetimeNluEntity;
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.Intent;
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.NluEntity;

import static org.junit.jupiter.api.Assertions.assertEquals;

@JsonTest
public class NluJsonSerializationTest {
    @Autowired
    private ObjectMapper objectMapper;

    @Test
    void testBooleanFieldsAreNull() throws Exception {
        String jsonFileLocation = "json/datetime_nlu_entity.json";
        Resource resource = new PathMatchingResourcePatternResolver().getResource(jsonFileLocation);
        try (InputStream inputStream = resource.getInputStream()) {
            String s = IOUtils.toString(inputStream, StandardCharsets.UTF_8);
            DatetimeNluEntity.Value actual = objectMapper.readValue(s, DatetimeNluEntity.Value.class);
            DatetimeNluEntity.Value expected = new DatetimeNluEntity.Value(
                    null, null, 1,
                    15, null,
                    null, null, true,
                    false, null
            );
            assertEquals(expected, actual);
        }
    }

    @Test
    void testYandexDatetimeSerialization() throws Exception {
        DatetimeNluEntity nluEntity = new DatetimeNluEntity(2, 9, new DatetimeNluEntity.Value(
                2020, 10, 16, 15, 0, false, false, false, false, false
        ));
        String s = objectMapper.writeValueAsString(nluEntity);
        NluEntity actual = objectMapper.readValue(s, NluEntity.class);
        assertEquals(nluEntity, actual);
    }

    @Test
    void testIntentSerialization() throws Exception {
        DatetimeNluEntity nluEntity = new DatetimeNluEntity(2, 9, new DatetimeNluEntity.Value(
                2020, 10, 16, 15, 0, false, false, false, false, false
        ));
        Intent intent = new Intent("ScheduleConf", Map.of("dta", nluEntity));
        String s = objectMapper.writeValueAsString(intent);
        SlotsMap actual = objectMapper.readValue(s, SlotsMap.class);
        assertEquals(intent.getSlots(), actual.slots());
    }

    private record SlotsMap(Map<String, NluEntity> slots) {
    }
}
