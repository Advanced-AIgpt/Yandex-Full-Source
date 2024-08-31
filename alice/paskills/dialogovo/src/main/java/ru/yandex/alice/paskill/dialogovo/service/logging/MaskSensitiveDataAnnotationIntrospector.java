package ru.yandex.alice.paskill.dialogovo.service.logging;

import java.io.IOException;

import javax.annotation.Nullable;

import com.fasterxml.jackson.core.JsonGenerator;
import com.fasterxml.jackson.databind.SerializerProvider;
import com.fasterxml.jackson.databind.introspect.Annotated;
import com.fasterxml.jackson.databind.introspect.NopAnnotationIntrospector;
import com.fasterxml.jackson.databind.ser.std.StdSerializer;

/**
 * The class is used to mask sensitive convent when serializing object for loggging
 *
 * @see SkillRequestLogger#log
 */
class MaskSensitiveDataAnnotationIntrospector extends NopAnnotationIntrospector {
    @Nullable
    @Override
    public Object findSerializer(Annotated am) {
        MaskSensitiveData annotation = am.getAnnotation(MaskSensitiveData.class);
        if (annotation != null && String.class.equals(am.getRawType())) {
            return MaskSensitiveDataSerializer.class;
        }

        return null;
    }

    @Nullable
    @Override
    public Object findContentSerializer(Annotated am) {
        MaskSensitiveData annotation = am.getAnnotation(MaskSensitiveData.class);
        if (annotation != null) {
            return MaskSensitiveDataSerializer.class;
        }

        return null;
    }

    static class MaskSensitiveDataSerializer extends StdSerializer<String> {

        MaskSensitiveDataSerializer() {
            super(String.class);
        }


        @Override
        public void serialize(String value, JsonGenerator gen, SerializerProvider provider) throws IOException {
            gen.writeString("*".repeat(value.length()));
        }
    }


}
