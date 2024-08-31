package ru.yandex.alice.paskill.dialogovo.external.v1.response.audio;

import javax.validation.constraints.NotNull;
import javax.validation.constraints.Size;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;
import org.hibernate.validator.constraints.URL;

import static com.fasterxml.jackson.annotation.JsonInclude.Include.NON_ABSENT;

@Data
@JsonInclude(NON_ABSENT)
public class AudioStream {
    @NotNull
    @Size(max = 1024)
    @URL(regexp = "^http(s)?.*")
    private final String url;

    @NotNull
    @JsonProperty(value = "offset_ms")
    private final long offsetMs;

    @NotNull
    @Size(max = 1024)
    @JsonProperty(value = "token")
    private final String token;

    public String getUrl() {
        return url;
    }

    public long getOffsetMs() {
        return offsetMs;
    }

    public String getToken() {
        return token;
    }
}
