package ru.yandex.quasar.billing.services.processing.trust;

import java.io.IOException;
import java.math.BigDecimal;
import java.time.Instant;

import com.fasterxml.jackson.core.JsonParser;
import com.fasterxml.jackson.core.JsonTokenId;
import com.fasterxml.jackson.databind.DeserializationContext;
import com.fasterxml.jackson.databind.deser.std.StdScalarDeserializer;

public class TrustDateDeserializer extends StdScalarDeserializer<Instant> {

    private static final BigDecimal THOUSAND = new BigDecimal("1000");

    public TrustDateDeserializer() {
        super(Instant.class);
    }

    @Override
    public Instant deserialize(JsonParser p, DeserializationContext ctxt) throws IOException {
        BigDecimal value = null;
        switch (p.getCurrentTokenId()) {
            case JsonTokenId.ID_NULL:
                return null;
            case JsonTokenId.ID_NUMBER_INT:
            case JsonTokenId.ID_NUMBER_FLOAT:
                value = p.getDecimalValue();
                break;
            case JsonTokenId.ID_STRING:
                String text = p.getText().trim();
                // note: no need to call `coerce` as this is never primitive
                if (_isEmptyOrTextualNull(text)) {
                    _verifyNullForScalarCoercion(ctxt, text);
                    return getNullValue(ctxt);
                }
                _verifyStringForScalarCoercion(ctxt, text);
                value = new BigDecimal(text);
                break;
            default:
        }

        return value != null ?
                Instant.ofEpochMilli(value.multiply(THOUSAND).longValue()) :
                null;
    }


}
