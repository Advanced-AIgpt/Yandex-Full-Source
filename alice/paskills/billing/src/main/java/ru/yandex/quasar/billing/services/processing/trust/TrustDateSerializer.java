package ru.yandex.quasar.billing.services.processing.trust;

import java.io.IOException;
import java.text.DecimalFormat;
import java.time.Instant;

import com.fasterxml.jackson.core.JsonGenerator;
import com.fasterxml.jackson.databind.SerializerProvider;
import com.fasterxml.jackson.databind.ser.std.StdScalarSerializer;

public class TrustDateSerializer extends StdScalarSerializer<Instant> {

    public TrustDateSerializer() {
        super(Instant.class);
    }

    @Override
    public void serialize(Instant value, JsonGenerator gen, SerializerProvider provider) throws IOException {
        long epochMillis = value.toEpochMilli();
        gen.writeString(String.valueOf(epochMillis / 1000L) + "." +
                new DecimalFormat("000").format(epochMillis % 1000));
    }
}
