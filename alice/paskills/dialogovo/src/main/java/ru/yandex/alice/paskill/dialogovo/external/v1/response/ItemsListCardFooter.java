package ru.yandex.alice.paskill.dialogovo.external.v1.response;

import java.util.Optional;

import javax.validation.Valid;
import javax.validation.constraints.Size;

import lombok.AllArgsConstructor;
import lombok.Data;

import ru.yandex.alice.paskill.dialogovo.domain.Censored;

@Data
@Censored
@AllArgsConstructor
public class ItemsListCardFooter {

    private final Optional<@Valid CardButton> button;
    @Censored
    private Optional<@Size(max = 128) String> text;

    public Optional<CardButton> getButton() {
        return button;
    }

    public Optional<String> getText() {
        return text;
    }
}
