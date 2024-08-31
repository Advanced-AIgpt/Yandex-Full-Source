package ru.yandex.alice.paskill.dialogovo.external.v1.response;

import javax.validation.constraints.NotNull;
import javax.validation.constraints.Size;

import lombok.Data;
import org.hibernate.validator.constraints.URL;

@Data
public class Image {

    @Size(max = 1024)
    @URL(regexp = "^http(s)?.*")
    @NotNull
    private final String url;

    public String getUrl() {
        return url;
    }
}
