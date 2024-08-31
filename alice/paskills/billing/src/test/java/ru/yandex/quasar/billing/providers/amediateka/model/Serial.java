package ru.yandex.quasar.billing.providers.amediateka.model;

import java.util.List;

import lombok.Data;
import lombok.Getter;

@Getter
public class Serial extends BaseObject {
    private List<Season> seasons;

    public void setSeasons(List<Season> seasons) {
        this.seasons = seasons;
    }

    @Data
    public static class SerialDTO {
        private Serial serial;
    }
}
