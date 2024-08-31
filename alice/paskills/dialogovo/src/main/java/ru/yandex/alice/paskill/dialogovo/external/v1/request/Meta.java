package ru.yandex.alice.paskill.dialogovo.external.v1.request;

import java.util.List;
import java.util.Optional;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Data;

@Data
@Builder
@AllArgsConstructor
@JsonInclude(JsonInclude.Include.NON_ABSENT)
public class Meta {
    private final String locale;
    private final String timezone;

    @JsonProperty("filtering_mode")
    private final FilteringMode filteringMode;

    @JsonProperty("client_id")
    private final String clientId;

    private final Interfaces interfaces;

    @JsonProperty("device_id")
    private final Optional<String> deviceId;

    @Builder.Default
    private final Optional<List<String>> flags = Optional.empty();
    @Builder.Default
    private final Optional<List<String>> experiments = Optional.empty();

    @Data
    @JsonInclude(JsonInclude.Include.NON_ABSENT)
    public static class Interfaces {

        private final Optional<Screen> screen;
        private final Optional<Payment> payments;
        @JsonProperty("account_linking")
        private final Optional<AccountLinking> accountLinking;
        @JsonProperty("audio_player")
        private final Optional<AudioPlayer> audioPlayer;
        @JsonProperty("music_product_activation")
        private final Optional<MusicProductActivation> musicProductActivation;
        @JsonProperty("geolocation_sharing")
        private final Optional<GeolocationSharing> geolocationSharing;
        @JsonProperty("user_agreements")
        private final Optional<UserAgreements> userAgreements;

        public static final Interfaces EMPTY_INSTANCE = new Interfaces(
                Optional.empty(), Optional.empty(), Optional.empty(),
                Optional.empty(), Optional.empty(), Optional.empty(),
                Optional.empty()
        );

        public static Interfaces of(boolean hasScreen,
                                    boolean supportsBilling,
                                    boolean supportsAccountLinking,
                                    boolean hasAudioPlayer,
                                    boolean supportMusicProductActivation,
                                    boolean supportGeolocationSharing,
                                    boolean supportsUserAgreements) {
            return new Interfaces(
                    hasScreen ? Optional.of(Screen.INSTANCE) : Optional.empty(),
                    supportsBilling ? Optional.of(Payment.INSTANCE) : Optional.empty(),
                    supportsAccountLinking ? Optional.of(AccountLinking.INSTANCE) : Optional.empty(),
                    hasAudioPlayer ? Optional.of(AudioPlayer.INSTANCE) : Optional.empty(),
                    supportMusicProductActivation ? Optional.of(MusicProductActivation.INSTANCE) : Optional.empty(),
                    supportGeolocationSharing ? Optional.of(GeolocationSharing.INSTANCE) : Optional.empty(),
                    supportsUserAgreements ? Optional.of(UserAgreements.INSTANCE) : Optional.empty());
        }

        @JsonSerialize
        public static class AccountLinking {
            public static final AccountLinking INSTANCE = new AccountLinking();
        }

        @JsonSerialize
        public static class MusicProductActivation {
            public static final MusicProductActivation INSTANCE = new MusicProductActivation();
        }

        @JsonSerialize
        public static class GeolocationSharing {
            public static final GeolocationSharing INSTANCE = new GeolocationSharing();
        }

        @JsonSerialize
        public static class UserAgreements {
            public static final UserAgreements INSTANCE = new UserAgreements();
        }

        @JsonSerialize
        public static class Payment {
            public static final Payment INSTANCE = new Payment();
        }

        @JsonSerialize
        public static class Screen {
            public static final Screen INSTANCE = new Screen();
        }

        @JsonSerialize
        public static class AudioPlayer {
            public static final AudioPlayer INSTANCE = new AudioPlayer();
        }
    }
}
