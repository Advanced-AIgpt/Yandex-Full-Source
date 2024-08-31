package ru.yandex.quasar.billing.beans;

import java.io.IOException;

import com.fasterxml.jackson.core.JsonParser;
import com.fasterxml.jackson.core.JsonToken;
import com.fasterxml.jackson.databind.DeserializationContext;
import com.fasterxml.jackson.databind.JsonDeserializer;

class LogicalPeriodDeserializer extends JsonDeserializer<LogicalPeriod> {
    @Override
    public LogicalPeriod deserialize(JsonParser parser, DeserializationContext ctxt) throws IOException {
        if (parser.hasToken(JsonToken.VALUE_NULL)) {
            return null;
        } else if (parser.hasToken(JsonToken.VALUE_STRING)) {
            String string = parser.getText().trim();
            if (string.length() == 0) {
                return null;
            }
            return LogicalPeriod.parse(string);
        }
        throw ctxt.wrongTokenException(parser, handledType(), JsonToken.VALUE_STRING, null);
    }
}
