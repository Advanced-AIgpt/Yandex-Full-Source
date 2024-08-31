package ru.yandex.quasar.billing.providers.amediateka.model;

import java.util.List;

import lombok.Getter;

@Getter
public class SeasonEpisodes extends MultipleDTO<Episode> {
    private List<Episode> episodes;

    @Override
    public List<Episode> getPayload() {
        return episodes;
    }
}
