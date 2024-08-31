package ru.yandex.alice.paskill.dialogovo.external.v1.nlu.json;

import java.io.IOException;
import java.math.BigDecimal;

import com.fasterxml.jackson.core.JsonParser;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.core.ObjectCodec;
import com.fasterxml.jackson.databind.DeserializationContext;
import com.fasterxml.jackson.databind.JsonDeserializer;
import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.springframework.boot.jackson.JsonComponent;

import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.BuiltinNluEntityType;
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.CustomEntity;
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.DatetimeNluEntity;
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.FioNluEntity;
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.GeoNluEntity;
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.NluEntity;
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.NumberNluEntity;
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.StringEntity;
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.UnknownBuiltinEntity;

@JsonComponent
public class NluEntityDeserializer extends JsonDeserializer<NluEntity> {

    @Override
    public NluEntity deserialize(JsonParser parser, DeserializationContext ctxt) throws IOException {
        ObjectCodec codec = parser.getCodec();
        JsonNode node = codec.readTree(parser);
        String type = node.get("type").asText("MISSING_ENTITY_TYPE");
        NluEntity.PositionTokens tokens = codec.treeToValue(node.get("tokens"),
                NluEntity.PositionTokens.class);
        JsonNode jsonValue = node.get("value");
        return parseValueWithType(type, jsonValue, tokens, codec);
    }

    public static NluEntity entityFromString(
            String type,
            int begin,
            int end,
            String serializedValue,
            ObjectMapper objectMapper
    ) throws JsonProcessingException {
        JsonNode jsonValue = objectMapper.readTree(serializedValue);
        return parseValueWithType(type, jsonValue, new NluEntity.PositionTokens(begin, end), objectMapper);
    }

    private static NluEntity parseValueWithType(
            String type,
            JsonNode valueJson,
            NluEntity.PositionTokens tokens,
            ObjectCodec codec
    ) throws JsonProcessingException {
        switch (type) {
            case BuiltinNluEntityType.DATETIME:
                return new DatetimeNluEntity(tokens.getStart(),
                        tokens.getEnd(),
                        codec.treeToValue(valueJson, DatetimeNluEntity.Value.class));
            case BuiltinNluEntityType.FIO:
                return new FioNluEntity(tokens.getStart(),
                        tokens.getEnd(),
                        codec.treeToValue(valueJson, FioNluEntity.Value.class));
            case BuiltinNluEntityType.GEO:
                return new GeoNluEntity(tokens.getStart(),
                        tokens.getEnd(),
                        codec.treeToValue(valueJson, GeoNluEntity.Value.class));
            case BuiltinNluEntityType.NUMBER:
                return new NumberNluEntity(tokens.getStart(),
                        tokens.getEnd(),
                        codec.treeToValue(valueJson, BigDecimal.class));
            case BuiltinNluEntityType.STRING:
                return new StringEntity(tokens.getStart(),
                        tokens.getEnd(),
                        codec.treeToValue(valueJson, String.class));
            default:
                if (type.startsWith("YANDEX.")) {
                    return new UnknownBuiltinEntity(tokens.getStart(), tokens.getEnd(), type, valueJson);
                } else {
                    String value = codec.treeToValue(valueJson, String.class);
                    return new CustomEntity(tokens.getStart(), tokens.getEnd(), type, value);
                }
        }
    }

}
