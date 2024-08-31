package ru.yandex.alice.paskill.dialogovo.external.v1.nlu.json;

import java.io.IOException;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

import javax.annotation.Nullable;

import com.fasterxml.jackson.core.JsonParser;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.core.ObjectCodec;
import com.fasterxml.jackson.databind.DeserializationContext;
import com.fasterxml.jackson.databind.JsonDeserializer;
import com.fasterxml.jackson.databind.JsonNode;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.Intent;
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.NluEntity;

public class IntentDeserializer extends JsonDeserializer<Map<String, Intent>> {

    private static final Logger logger = LogManager.getLogger();

    private static final String SLOTS_KEY = "slots";

    @Override
    public Map<String, Intent> deserialize(JsonParser parser, DeserializationContext ctxt) throws IOException {
        ObjectCodec codec = parser.getCodec();
        JsonNode node = codec.readTree(parser);
        if (node.isObject()) {
            Map<String, Intent> intents = new HashMap<>(node.size());
            Iterator<Map.Entry<String, JsonNode>> intentIterator = node.fields();
            while (intentIterator.hasNext()) {
                Map.Entry<String, JsonNode> serializedIntentWithName = intentIterator.next();
                String name = serializedIntentWithName.getKey();
                JsonNode serializedIntent = serializedIntentWithName.getValue();
                intents.put(name, new Intent(name, parseSlots(serializedIntent.get(SLOTS_KEY), codec)));
            }
            return intents;
        } else {
            logger.error("Failed to parse intents: expected JSON object, got {}", node.getNodeType());
            return Collections.emptyMap();
        }
    }

    private Map<String, NluEntity> parseSlots(
            @Nullable JsonNode serializedSlots,
            ObjectCodec codec
    ) throws JsonProcessingException {
        if (serializedSlots == null || !serializedSlots.isObject()) {
            return Collections.emptyMap();
        }
        Map<String, NluEntity> slots = new HashMap<>(serializedSlots.size());
        Iterator<Map.Entry<String, JsonNode>> slotsEntries = serializedSlots.fields();
        while (slotsEntries.hasNext()) {
            Map.Entry<String, JsonNode> serializedSlot = slotsEntries.next();
            slots.put(serializedSlot.getKey(), codec.treeToValue(serializedSlot.getValue(), NluEntity.class));
        }
        return slots;
    }
}
