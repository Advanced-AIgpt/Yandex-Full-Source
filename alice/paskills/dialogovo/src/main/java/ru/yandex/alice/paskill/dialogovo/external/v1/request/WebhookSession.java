package ru.yandex.alice.paskill.dialogovo.external.v1.request;

import java.util.List;
import java.util.Optional;
import java.util.UUID;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Data;
import lombok.NonNull;

import ru.yandex.alice.kronstadt.core.domain.LocationInfo;
import ru.yandex.alice.paskill.dialogovo.service.logging.MaskSensitiveData;

import static com.fasterxml.jackson.annotation.JsonInclude.Include.NON_ABSENT;
import static com.fasterxml.jackson.annotation.JsonInclude.Include.NON_EMPTY;

@Data
@Builder
@AllArgsConstructor
@JsonInclude(NON_ABSENT)
public class WebhookSession {
    @JsonProperty(value = "message_id")
    private final long messageId;
    @JsonProperty("event_id")
    private final Optional<UUID> eventId;
    @JsonProperty("session_id")
    private final String sessionId;
    @JsonProperty("skill_id")
    private final String skillId;
    private final Optional<User> user;
    private final Application application;
    @JsonProperty("location")
    @JsonInclude(NON_ABSENT)
    private final Optional<Location> location;

    @JsonProperty("floyd_user")
    @JsonInclude(NON_ABSENT)
    private final Optional<FloydUser> floydUser;

    @JsonProperty("new")
    public boolean getIsNew() {
        return messageId == 0;
    }

    @Deprecated
    @JsonProperty("user_id")
    public String getUserId() {
        return this.application.applicationId;
    }

    @Data
    public static class User {
        @JsonProperty("user_id")
        @NonNull
        private final String userId;
        @JsonProperty("access_token")
        @JsonInclude(NON_ABSENT)
        @MaskSensitiveData
        private final Optional<String> accessToken;
        @JsonProperty("skill_products")
        @JsonInclude(NON_EMPTY)
        private final List<UserSkillProducts> userSkillProducts;
        @JsonProperty("accepted_user_agreements")
        @JsonInclude(NON_ABSENT)
        private final Optional<Boolean> agreedToUserAgreements;

        /*Access_token must be masked!*/
        @Override
        public String toString() {
            return "User{" +
                    "userId='" + userId + '\'' +
                    ", accessToken=" + accessToken.map(it -> "*****").orElse("null") +
                    ", agreedToUserAgreements=" + agreedToUserAgreements.toString() +
                    "}";
        }
    }

    /**
     * Used for floyd skills calls only
     */
    @JsonInclude(NON_ABSENT)
    @Data
    public static class FloydUser {

        @Nullable
        @JsonProperty("puid")
        @JsonInclude(NON_EMPTY)
        private final String floydUserPuid;
        @Nullable
        @JsonProperty("login")
        @JsonInclude(NON_EMPTY)
        private final String floydUserLogin;

        @Nullable
        @JsonProperty("operator_chat_id")
        @JsonInclude(NON_EMPTY)
        private final String operatorChatId;
    }

    @Data
    public static class UserSkillProducts {
        private final String uuid;
        private final String name;
    }

    @Data
    public static class Location {
        private final double lat;
        private final double lon;
        private final double accuracy;

        public static Optional<Location> from(Optional<LocationInfo> locationInfo) {
            return locationInfo.map(loc -> new Location(loc.getLat(), loc.getLon(), loc.getAccuracy()));
        }
    }

    @Data
    public static class Application {
        @JsonProperty("application_id")
        private final String applicationId;
    }
}
