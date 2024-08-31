package ru.yandex.alice.paskill.dialogovo.external.v1.response;

import java.util.List;
import java.util.Optional;

import javax.validation.Valid;
import javax.validation.constraints.NotNull;
import javax.validation.constraints.Size;

import lombok.Data;
import lombok.EqualsAndHashCode;

@Data
@EqualsAndHashCode(callSuper = true)
public class ItemsListCard extends Card {

    // https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/forms/external_skill/scheme.sc?rev=5781373#L205
    @Valid
    @NotNull
    @Size(min = 1, max = 5)
    private final List<ItemsListItem> items;
    private final Optional<@Valid ItemsListCardHeader> header;
    private final Optional<@Valid ItemsListCardFooter> footer;

    public List<ItemsListItem> getItems() {
        return items;
    }

    public Optional<ItemsListCardHeader> getHeader() {
        return header;
    }

    public Optional<ItemsListCardFooter> getFooter() {
        return footer;
    }
}
