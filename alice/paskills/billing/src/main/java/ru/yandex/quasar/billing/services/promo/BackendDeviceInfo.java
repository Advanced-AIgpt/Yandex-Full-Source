package ru.yandex.quasar.billing.services.promo;

import java.io.IOException;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.time.Instant;
import java.time.format.DateTimeFormatter;
import java.util.Collections;
import java.util.Set;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.core.JsonParseException;
import com.fasterxml.jackson.core.JsonParser;
import com.fasterxml.jackson.databind.DeserializationContext;
import com.fasterxml.jackson.databind.JsonDeserializer;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import lombok.AllArgsConstructor;
import lombok.Data;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.apache.logging.log4j.util.Strings;

@Data
@AllArgsConstructor
@JsonIgnoreProperties(ignoreUnknown = true)
public class BackendDeviceInfo {
    private final String id;
    @JsonProperty("platform")
    private final Platform platform;
    private final Set<String> tags;

    @JsonProperty("name")
    private final String name;
    @JsonProperty("activation_region")
    @Nullable
    private final String activationRegion;

    // next 3 fields are for temporary use while migrating promo activation from droideka
    @Nullable
    @JsonProperty("serial")
    private final String serial;

    @Nullable
    @JsonProperty("wifi_mac")
    private final String wifiMac;

    @Nullable
    @JsonProperty("ethernet_mac")
    private final String ethernetMac;

    @JsonProperty("first_activation_date")
    @JsonDeserialize(using = DateDeserializer.class)
    @Nullable
    private final Instant firstActivation;

    public BackendDeviceInfo(String id, Platform platform, Set<String> tags, String name,
                             @Nullable String activationRegion) {
        this.id = id;
        this.platform = platform;
        this.tags = tags;
        this.name = name;
        this.activationRegion = activationRegion;
        this.serial = null;
        this.wifiMac = null;
        this.ethernetMac = null;
        this.firstActivation = Instant.EPOCH;
    }

    public BackendDeviceInfo(String id, Platform platform, Set<String> tags, String name,
                             @Nullable String activationRegion, @Nullable Instant firstActivation) {
        this.id = id;
        this.platform = platform;
        this.tags = tags;
        this.name = name;
        this.activationRegion = activationRegion;
        this.serial = null;
        this.wifiMac = null;
        this.ethernetMac = null;
        this.firstActivation = firstActivation;
    }

    public static BackendDeviceInfo create(
            String id, Platform platform, Set<String> tags, @Nullable String activationRegion
    ) {
        return new BackendDeviceInfo(id, platform, tags, null, activationRegion, null, null, null, Instant.EPOCH);
    }

    public static BackendDeviceInfo create(
            String id, Platform platform, Set<String> tags, @Nullable String activationRegion,
            String serial, String wifiMac, String ethernetMac
    ) {
        return new BackendDeviceInfo(id, platform, tags, null, activationRegion, serial, wifiMac, ethernetMac,
                Instant.EPOCH);
    }

    public Set<String> getTags() {
        return tags != null ? tags : Collections.emptySet();
    }

    @JsonIgnore
    public DeviceId getDeviceIdentifier() {
        return DeviceId.create(id, platform);
    }

    // from backend empty region comes as ""
    @Nullable
    @JsonIgnore
    public String getActivationRegion() {
        return Strings.trimToNull(activationRegion);
    }

    @JsonIgnore
    public boolean isDroidekaMaintainedDevice() {
        return Strings.isNotEmpty(wifiMac) &&
                Strings.isNotEmpty(ethernetMac) &&
                // module 2 never used droideka for promo activation, no need to wait for it
                !Platform.YANDEXMODULE_2.equals(platform);
    }

    public BackendDeviceInfo withFirstActivationTime(Instant newFirstActivation) {
        return new BackendDeviceInfo(id, platform, tags, name, activationRegion, serial, wifiMac, ethernetMac,
                newFirstActivation);
    }

    private static class NumericBooleanDeserializer extends JsonDeserializer<Boolean> {
        @Override
        public Boolean deserialize(JsonParser parser, DeserializationContext context) throws IOException {
            switch (parser.getCurrentToken()) {
                case VALUE_TRUE:
                    return true;
                case VALUE_FALSE:
                    return false;
                case VALUE_NUMBER_INT:
                    return 1 == parser.getIntValue();
                case VALUE_STRING:
                    return "1".equals(parser.getText());
                default:
                    throw new JsonParseException(parser, "Wrong type token");
            }

        }

    }

    private static class DateDeserializer extends JsonDeserializer<Instant> {
        private static final Logger logger = LogManager.getLogger();

        @Nullable
        @Override
        public Instant deserialize(JsonParser parser, DeserializationContext context) throws IOException {
            //Remove try when quasar backend will
            try {
                if (parser.getText().isEmpty()) {
                    return null;
                }
                return Instant.from(DateTimeFormatter.ISO_INSTANT.parse(parser.getText()));
            } catch (Exception e) {
                try {
                    return new SimpleDateFormat("yyyy-MM-dd hh:mm:ss")
                            .parse(parser.getText())
                            .toInstant();
                } catch (ParseException ex) {
                    logger.error("Can't parse date from Backend device info: ", ex);
                    throw new RuntimeException(ex);
                }
            }
        }
    }

    public static class Region {
        public static final String RU = "RU";
        public static final String KZ = "KZ";
        public static final String BY = "BY";
        public static final String IL = "IL";
        public static final String AZ = "AZ";
        public static final String UZ = "UZ";
    }
}
