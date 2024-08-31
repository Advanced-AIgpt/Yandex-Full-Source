package ru.yandex.alice.paskills.my_alice.bunker.client;

import javax.annotation.Nullable;

import lombok.Data;
import lombok.NonNull;
import org.springframework.http.MediaType;

@Data
@NonNull
public class NodeInfo {
    private final String path;
    @Nullable
    private final MediaType mediaType;
}
