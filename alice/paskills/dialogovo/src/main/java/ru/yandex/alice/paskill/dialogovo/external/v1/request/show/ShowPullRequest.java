package ru.yandex.alice.paskill.dialogovo.external.v1.request.show;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Getter;

import ru.yandex.alice.paskill.dialogovo.external.v1.request.InputType;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.RequestBase;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.show.ShowType;

import static com.fasterxml.jackson.annotation.JsonInclude.Include.NON_ABSENT;

@JsonInclude(NON_ABSENT)
@Getter
public class ShowPullRequest extends RequestBase {
    @JsonProperty("show_type")
    private final ShowType showType;

    public ShowPullRequest(
            ShowType showType
    ) {
        super(InputType.SHOW_PULL_REQUEST);
        this.showType = showType;
    }
}
