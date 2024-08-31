package ru.yandex.alice.paskill.dialogovo.external.v1.nlu.json;

import java.io.IOException;

import com.fasterxml.jackson.core.JsonGenerator;
import com.fasterxml.jackson.databind.JsonSerializer;
import com.fasterxml.jackson.databind.SerializerProvider;
import org.springframework.boot.jackson.JsonComponent;

import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.NluEntity;
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.UnknownBuiltinEntity;

@JsonComponent
public class NluEntitySerializer extends JsonSerializer<NluEntity> {

    @Override
    public void serialize(NluEntity entity, JsonGenerator gen, SerializerProvider provider) throws IOException {
        if (!(entity instanceof UnknownBuiltinEntity)) {
            gen.writeStartObject();
            gen.writeStringField("type", entity.getType());
            gen.writeObjectField("tokens", entity.getTokens());
            gen.writeObjectField("value", entity.getValue());
            if (!entity.getAdditionalValues().isEmpty()) {
                gen.writeObjectField("additional_values", entity.getAdditionalValues());
            }
            gen.writeEndObject();
        }
    }
}
