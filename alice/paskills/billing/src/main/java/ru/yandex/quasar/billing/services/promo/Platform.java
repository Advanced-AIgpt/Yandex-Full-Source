package ru.yandex.quasar.billing.services.promo;

import java.io.IOException;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.stream.Stream;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.core.JsonGenerator;
import com.fasterxml.jackson.core.JsonParseException;
import com.fasterxml.jackson.core.JsonParser;
import com.fasterxml.jackson.core.JsonToken;
import com.fasterxml.jackson.databind.DeserializationContext;
import com.fasterxml.jackson.databind.JsonDeserializer;
import com.fasterxml.jackson.databind.JsonSerializer;
import com.fasterxml.jackson.databind.SerializerProvider;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import lombok.EqualsAndHashCode;
import org.springframework.core.convert.converter.Converter;
import org.springframework.stereotype.Component;

import static java.util.stream.Collectors.toMap;

/**
 * Yandex device platform. To make possible seamlessly deserialize previously
 * unknown platforms (new ones) implement it via class not enum. Predefined static fields
 * {@see Platform.yandexstation} {@see Platform.yandexmodule} etc. may be compared via '=='.
 */
@JsonSerialize(using = Platform.PlatformSerializer.class)
@JsonDeserialize(using = Platform.PlatformDeserializer.class)
@EqualsAndHashCode
public class Platform {
    public static final Platform YANDEXSTATION = new Platform("yandexstation");
    public static final Platform YANDEXSTATION_2 = new Platform("yandexstation_2");
    public static final Platform YANDEXMINI = new Platform("yandexmini");
    public static final Platform YANDEXMIDI = new Platform("yandexmidi");
    public static final Platform YANDEXMODULE = new Platform("yandexmodule");
    public static final Platform YANDEXMODULE_2 = new Platform("yandexmodule_2");
    public static final Platform YANDEXMICRO = new Platform("yandexmicro"); // Яндекс Станция лайт
    public static final Platform LINKPLAY_A98 = new Platform("linkplay_a98");
    public static final Platform LIGHTCOMM = new Platform("lightcomm");
    public static final Platform WK7Y = new Platform("wk7y");
    public static final Platform ELARI_A98 = new Platform("elari_a98");
    public static final Platform JET_SMART_MUSIC = new Platform("jet_smart_music");
    public static final Platform PRESTIGIO_SMART_MATE = new Platform("prestigio_smart_mate");
    public static final Platform DIGMA_DI_HOME = new Platform("digma_di_home");
    public static final Platform YANDEXMINI_2 = new Platform("yandexmini_2");

    public static final Platform YANDEX_TV_RT2871_HIKEEN = new Platform("yandex_tv_rt2871_hikeen");
    public static final Platform YANDEX_TV_RT2861_HIKEEN = new Platform("yandex_tv_rt2861_hikeen");
    public static final Platform YANDEX_TV_HISI351_CVTE = new Platform("yandex_tv_hisi351_cvte");
    public static final Platform YANDEX_TV_MT9632_CV = new Platform("yandex_tv_mt9632_cv");
    public static final Platform YANDEX_TV_MT6681_CV = new Platform("yandex_tv_mt6681_cv");
    public static final Platform YANDEX_TV_MT6681_CVTE = new Platform("yandex_tv_mt6681_cvte");
    public static final Platform YANDEX_TV_MT9632_CVTE = new Platform("yandex_tv_mt9632_cvte");
    public static final Platform YANDEX_TV_RT2842_HIKEEN = new Platform("yandex_tv_rt2842_hikeen");
    public static final Platform YANDEX_TV_MT9256_CVTE = new Platform("yandex_tv_mt9256_cvte");


    private static final Map<String, Platform> KNOWN_PLATFORMS = Stream.of(
            YANDEXSTATION,
            YANDEXSTATION_2,
            YANDEXMINI,
            YANDEXMIDI,
            YANDEXMODULE,
            YANDEXMODULE_2,
            YANDEXMICRO,
            LINKPLAY_A98,
            LIGHTCOMM,
            WK7Y,
            ELARI_A98,
            JET_SMART_MUSIC,
            PRESTIGIO_SMART_MATE,
            DIGMA_DI_HOME,
            YANDEXMINI_2,
            YANDEX_TV_RT2871_HIKEEN,
            YANDEX_TV_RT2861_HIKEEN,
            YANDEX_TV_HISI351_CVTE,
            YANDEX_TV_MT9632_CV,
            YANDEX_TV_MT6681_CV,
            YANDEX_TV_MT6681_CVTE,
            YANDEX_TV_MT9632_CVTE,
            YANDEX_TV_RT2842_HIKEEN,
            YANDEX_TV_MT9256_CVTE
    ).collect(toMap(x -> x.name, x -> x));

    public static final Set<Platform> TV_PLATFORMS = Set.of(
            YANDEX_TV_RT2871_HIKEEN,
            YANDEX_TV_RT2861_HIKEEN,
            YANDEX_TV_HISI351_CVTE,
            YANDEX_TV_MT9632_CV,
            YANDEX_TV_MT6681_CV,
            YANDEX_TV_MT6681_CVTE,
            YANDEX_TV_MT9632_CVTE,
            YANDEX_TV_RT2842_HIKEEN,
            YANDEX_TV_MT9256_CVTE
    );

    private final String name;

    private Platform(String name) {
        this.name = name;
    }

    public static Platform create(String name) {
        return Optional.ofNullable(KNOWN_PLATFORMS.get(name)).orElseGet(() -> new Platform(name));
    }

    public String getName() {
        return name;
    }

    @Override
    public String toString() {
        return name;
    }

    @JsonIgnore
    public boolean isTv() {
        return name.startsWith("yandex_tv_");
    }

    public static class PlatformSerializer extends JsonSerializer<Platform> {
        @Override
        public void serialize(Platform value, JsonGenerator gen, SerializerProvider serializers) throws IOException {
            gen.writeString(value.name);
        }
    }

    public static class PlatformDeserializer extends JsonDeserializer<Platform> {
        @Override
        public Platform deserialize(JsonParser p, DeserializationContext ctxt) throws IOException {
            if (p.currentToken() != JsonToken.VALUE_STRING) {
                throw new JsonParseException(p, String.format("Platform should be %s only",
                        JsonToken.VALUE_STRING.name()));
            }
            String name = p.readValueAs(String.class);
            return Platform.create(name);
        }
    }

    @Component
    static class StringToPlatformConverter implements Converter<String, Platform> {
        @Override
        public Platform convert(String source) {
            return Platform.create(source);
        }
    }

    @Component
    static class PlatformToStringConverter implements Converter<Platform, String> {
        @Override
        public String convert(Platform source) {
            return source.name;
        }
    }
}
