package ru.yandex.alice.paskill.dialogovo.external.v1.response;

import java.util.Map;
import java.util.Optional;

import javax.validation.Valid;
import javax.validation.constraints.NotNull;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Data;

import ru.yandex.alice.paskill.dialogovo.external.ApiVersion;

import static com.fasterxml.jackson.annotation.JsonInclude.Include.NON_ABSENT;

@Data
@JsonInclude(NON_ABSENT)
@AllArgsConstructor
@Builder(toBuilder = true)
public class WebhookResponse {
    private final Optional<@Valid Response> response;

    @Deprecated(forRemoval = true)
    @JsonProperty("start_account_linking")
    private final Optional<StartAccountLinking> startAccountLinking;

    @NotNull
    private final ApiVersion version;

    @JsonProperty("session_state")
    private final Optional<Map<String, Object>> sessionState;
    /**
     * On keepState we do not update or reset session and user state
     * Mostly applicable for fallback/static answers. Exp:
     * {
     * "response": {
     * "text": "fallback",
     * "end_session": false
     * },
     * "keepState": true,
     * "version": "1.0"
     * }
     * <p>
     * Not public option - enables with SkillFeatureFlag#DO_NOT_RESET_STATE_ON_KEEP_STATE
     * TODO: think about better place in answer
     */
    private final boolean keepState;
    @JsonProperty("user_state_update")
    private final Optional<Map<String, Object>> userStateUpdate;

    @JsonProperty("application_state")
    private final Optional<Map<String, Object>> applicationState;

    @JsonProperty("analytics")
    private final Optional<ResponseAnalytics> analytics;

    public Optional<Response> getResponse() {
        return response;
    }

    public Optional<StartAccountLinking> getStartAccountLinking() {
        return startAccountLinking;
    }

    public ApiVersion getVersion() {
        return version;
    }

    public Optional<Map<String, Object>> getSessionState() {
        return sessionState;
    }

    public boolean isKeepState() {
        return keepState;
    }

    public Optional<Map<String, Object>> getUserStateUpdate() {
        return userStateUpdate;
    }

    public Optional<Map<String, Object>> getApplicationState() {
        return applicationState;
    }

    public Optional<ResponseAnalytics> getAnalytics() {
        return analytics;
    }

    @JsonIgnore
    public boolean isStartAccountLinkingResponse() {
        return getStartAccountLinking().isPresent() ||
                response.flatMap(Response::getDirectives)
                        .flatMap(Directives::getStartAccountLinking)
                        .isPresent();
    }
}
