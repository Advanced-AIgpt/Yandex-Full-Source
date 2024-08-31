package ru.yandex.alice.paskills.my_alice.blackbox;

import java.math.BigInteger;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;
import java.util.stream.Stream;

import com.google.gson.annotations.SerializedName;
import lombok.Builder;
import lombok.Data;
import lombok.NonNull;
import lombok.ToString;
import org.springframework.lang.Nullable;

import static java.util.stream.Collectors.toMap;

public class SessionId {
    protected static class DbFields {
        static final String SUID_MAIL = "subscription.suid.2";
        static final String SUID_BETATEST = "subscription.suid.668";
        static final String SUID_YASTAFF = "subscription.suid.669";

        static final String USERINFO_FIRSTNAME = "userinfo.firstname.uid";
        static final String USERINFO_LASTNAME = "userinfo.lastname.uid";
    }

    protected static class Attributes {
        static final String NORMALIZED_LOGIN = "1008";
        static final String HAVE_PLUS = "1015";
        static final String KINOPOISK_OTT_SUBSCRIPTION_NAME = "1016";
    }

    protected static class PhoneAttributes {
        static final String E164_NUMBER = "102";
        static final String IS_DEFAULT = "107";
        static final String IS_SECURED = "108";
    }

    protected static class Aliases {
        static final String YANDEXOID = "13";
    }

    @Data
    @NonNull
    @ToString(exclude = {"tvmTicket", "rawData"})
    public static class Response {
        public static final Response EMPTY = new Response(Status.INVALID, "No response", null, null, null);

        private final Status status;
        @Nullable
        private final String error;
        @Nullable
        private final User user;
        @Nullable
        private final String tvmTicket;
        @Nullable
        private final RawResponse rawData;

        public static Response fromRaw(RawResponse raw) {
            var statusO = Optional.ofNullable(raw.getStatus())
                    .map(RawResponse.AuthStatus::getId)
                    .flatMap(Status::byValue);
            if (statusO.isEmpty()) {
                return new Response(
                        Status.INVALID,
                        "Unknown status value",
                        null,
                        null,
                        raw
                );
            }

            if (statusO.get().ordinal() > Status.NEED_RESET.ordinal()) {
                return new Response(
                        statusO.get(),
                        raw.getError(),
                        null,
                        null,
                        raw
                );
            }

            var userO = getUser(raw);
            if (userO.isEmpty()) {
                return new Response(
                        Status.INVALID,
                        "Failed to collect user data",
                        null,
                        null,
                        raw
                );
            }

            return new Response(
                    statusO.get(),
                    null,
                    userO.get(),
                    raw.getUserTicket(),
                    raw
            );
        }

        protected static Optional<User> getUser(RawResponse raw) {
            var uid = Optional.ofNullable(raw.getUid())
                    .map(RawResponse.Uid::getValue);
            if (uid.isEmpty()) {
                return Optional.empty();
            }

            return Optional.of(new User(
                    uid.get(),
                    raw.getLogin(),
                    raw.getDisplayName().getName(),
                    raw.getDisplayName().getAvatar(),
                    new User.Attributes(
                            "1".equals(raw.getAttributes().get(Attributes.HAVE_PLUS)),
                            "1".equals(raw.getDbFields().get(DbFields.SUID_BETATEST)),
                            raw.getAliases().get(Aliases.YANDEXOID)
                    )
            ));

        }

        public enum Status {
            VALID(0),
            NEED_RESET(1),
            EXPIRED(2),
            NOAUTH(3),
            DISABLED(4),
            INVALID(5);

            private static final Map<Integer, Status> VALUE_MAP = Stream.of(Status.values())
                    .collect(toMap(it -> it.value, it -> it));
            private final Integer value;

            Status(final Integer value) {
                this.value = value;
            }

            protected static Optional<Status> byValue(@Nullable Integer value) {
                return Optional.ofNullable(value)
                        .map(VALUE_MAP::get);
            }
        }
    }

    @Data
    @Builder
    public static class RawResponse {
        @Nullable
        private final AuthStatus status;
        @Nullable
        private final String error;
        @Nullable
        private final Integer age;
        @Nullable
        @SerializedName("expires_in")
        private final Integer expiresIn;
        @Nullable
        private final Integer ttl;
        @Nullable
        @SerializedName("default_uid")
        private final BigInteger defaultUid;
        @Nullable
        private final Uid uid;
        @Nullable
        private final String login;
        @Nullable
        private final Map<String, String> aliases;
        @Nullable
        private final Karma karma;
        @Nullable
        @SerializedName("karma_status")
        private final KarmaStatus karmaStatus;
        @Nullable
        @SerializedName("display_name")
        private final DisplayName displayName;
        @Nullable
        @SerializedName("dbfields")
        private final Map<String, String> dbFields;
        @Nullable
        private final Map<String, String> attributes;
        @Nullable
        private final List<Phone> phones;
        @Nullable
        private final List<Email> emails;
        @Nullable
        @SerializedName("address-list")
        private final List<Address> addressList;
        @Nullable
        @SerializedName("user_ticket")
        private final String userTicket;

        public Map<String, String> getAliases() {
            return Objects.requireNonNullElse(aliases, Map.of());
        }

        public DisplayName getDisplayName() {
            return Objects.requireNonNullElse(displayName, DisplayName.EMPTY);
        }

        public Map<String, String> getDbFields() {
            return Objects.requireNonNullElse(dbFields, Map.of());
        }

        public Map<String, String> getAttributes() {
            return Objects.requireNonNullElse(attributes, Map.of());
        }

        @Data
        public static class AuthStatus {
            @Nullable
            private final String value;
            @Nullable
            private final Integer id;
        }

        @Data
        public static class Uid {
            @Nullable
            private final BigInteger value;
            @Nullable
            private final Boolean hosted;
            @Nullable
            private final String domain;
        }

        @Data
        public static class Karma {
            @Nullable
            private final Integer value;
            @Nullable
            @SerializedName("allow-until")
            private final BigInteger allowUntil;
        }

        @Data
        public static class KarmaStatus {
            @Nullable
            private final Integer value;
        }

        @Data
        public static class DisplayName {
            protected static final DisplayName EMPTY = new DisplayName(null, null, null);

            @Nullable
            private final String name;
            @Nullable
            @SerializedName("public_name")
            private final String publicName;
            @Nullable
            private final User.Avatar avatar;

            public User.Avatar getAvatar() {
                return Objects.requireNonNullElse(avatar, User.Avatar.EMPTY);
            }
        }

        @Data
        public static class Phone {
            @Nullable
            private final String id;
            @Nullable
            private final Map<String, String> attributes;
        }

        @Data
        public static class Email {
            @Nullable
            private final String id;
            @Nullable
            private final Map<String, String> attributes;
        }

        @Data
        public static class Address {
            @Nullable
            private final String address;
            @Nullable
            private final Boolean validated;
            @Nullable
            @SerializedName("default")
            private final Boolean isDefault;
            @Nullable
            @SerializedName("native")
            private final Boolean isNative;
            @Nullable
            @SerializedName("born-date")
            private final String bornDate;
        }
    }
}
