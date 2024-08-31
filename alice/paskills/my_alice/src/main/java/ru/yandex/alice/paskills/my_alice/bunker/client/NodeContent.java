package ru.yandex.alice.paskills.my_alice.bunker.client;

import lombok.Data;
import lombok.NonNull;
import org.springframework.http.MediaType;

@Data
@NonNull
public class NodeContent<T> {
    private final MediaType mediaType;
    private final T content;
}
