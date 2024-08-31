package ru.yandex.alice.paskill.dialogovo.external.v1.response;

import java.util.Optional;

import javax.annotation.Nullable;
import javax.validation.constraints.NotNull;
import javax.validation.constraints.Size;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonRawValue;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import lombok.AllArgsConstructor;
import lombok.Data;
import org.hibernate.validator.constraints.URL;

import ru.yandex.alice.kronstadt.core.utils.AnythingToStringJacksonDeserializer;
import ru.yandex.alice.paskill.dialogovo.domain.Censored;
import ru.yandex.alice.paskill.dialogovo.utils.SizeInBytes;

import static com.fasterxml.jackson.annotation.JsonInclude.Include.NON_ABSENT;

@Data
@Censored
@AllArgsConstructor
@JsonInclude(NON_ABSENT)
public class Button {
    @SizeInBytes(max = 4096)
    @JsonRawValue
    @JsonDeserialize(using = AnythingToStringJacksonDeserializer.class)
    @Nullable
    private final String payload;
    private final Optional<@Size(max = 1024) @URL(regexp = "^http(s)?.*") String> url;
    @Nullable
    private final Boolean hide;
    @Size(max = 128)
    @NotNull
    @Censored
    private String title;

    @Nullable
    public String getPayload() {
        return payload;
    }

    public Optional<String> getUrl() {
        return url;
    }

    public String getTitle() {
        return title;
    }

    public boolean getHide() {
        return hide != null && hide;
    }
}
