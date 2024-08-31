package ru.yandex.alice.memento.scanner;

import java.util.Map;
import java.util.function.BiConsumer;

import com.google.protobuf.Any;
import com.google.protobuf.GeneratedMessageV3;
import com.google.protobuf.ProtocolMessageEnum;
import lombok.AccessLevel;
import lombok.Data;
import lombok.Getter;

@Data
@Getter(AccessLevel.PRIVATE)
public class KeyMapping<T extends ProtocolMessageEnum> {

    private final Map<T, String> dbKeys;
    private final Map<String, T> enums;
    private final Map<T, String> keyToTypeUrl;
    private final T unrecognized;
    private final Map<T, Any> defaultMessages;
    private final Map<T, Class<GeneratedMessageV3>> keyToClass;
    private final Map<T, BiConsumer<GeneratedMessageV3.Builder<?>, GeneratedMessageV3>> fieldSetterMap;
    private final Map<T, GeneratedMessageV3> explicitDefaults;

    public String getDbKey(T enumKey) {
        return dbKeys.get(enumKey);
    }

    public T getEnumKey(String key) {
        return enums.getOrDefault(key, unrecognized);
    }

    public String getTypeUrl(T key) {
        return keyToTypeUrl.get(key);
    }

    public String getTypeUrl(String key) {
        return keyToTypeUrl.get(enums.getOrDefault(key, unrecognized));
    }

    public Any getDefault(T key) {
        return defaultMessages.get(key);
    }

    public Class<GeneratedMessageV3> getClass(T key) {
        return keyToClass.get(key);
    }

    public void setField(T key, GeneratedMessageV3.Builder<?> builder, GeneratedMessageV3 value) {
        fieldSetterMap.get(key).accept(builder, value);
    }

    public Map<T, GeneratedMessageV3> getExplicitDefaults() {
        return explicitDefaults;
    }
}
