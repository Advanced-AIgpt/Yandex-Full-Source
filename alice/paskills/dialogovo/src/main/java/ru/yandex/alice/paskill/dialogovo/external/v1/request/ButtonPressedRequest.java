package ru.yandex.alice.paskill.dialogovo.external.v1.request;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import lombok.Getter;

import ru.yandex.alice.kronstadt.core.utils.AnythingFromStringJacksonSerializer;
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.Nlu;

@Getter
public class ButtonPressedRequest extends UtteranceRequestBase {
    @Nullable
    @JsonSerialize(using = AnythingFromStringJacksonSerializer.class)
    private final Object payload;

    public ButtonPressedRequest(@JsonProperty("nlu") Nlu nlu,
                                @Nullable @JsonProperty("payload") Object payload) {

        super(InputType.BUTTON_PRESSED, nlu);
        this.payload = payload;
    }
}
