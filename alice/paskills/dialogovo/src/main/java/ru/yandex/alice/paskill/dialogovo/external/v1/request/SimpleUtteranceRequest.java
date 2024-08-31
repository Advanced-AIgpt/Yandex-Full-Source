package ru.yandex.alice.paskill.dialogovo.external.v1.request;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Getter;

import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.Nlu;

@Getter
public class SimpleUtteranceRequest extends UtteranceRequestBase {
    private final Markup markup;
    @Nullable
    private final String command;

    @JsonProperty("original_utterance")
    @Nullable
    private final String originalUtterance;

    @JsonCreator
    public SimpleUtteranceRequest(@Nullable @JsonProperty("command") String command,
                                  @Nullable @JsonProperty("original_utterance") String originalUtterance,
                                  @JsonProperty("nlu") Nlu nlu,
                                  @JsonProperty("markup") Markup markup) {

        super(InputType.SIMPLE_UTTERANCE, nlu);
        this.command = command;
        this.originalUtterance = originalUtterance;
        this.markup = markup;
    }
}
