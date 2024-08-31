package ru.yandex.quasar.billing.providers.amediateka.model;

import ru.yandex.quasar.billing.beans.ContentMetaInfo;

public class SerialMetaInfo extends BaseMetaInfo {

    private String[] country;

    private SeasonMetaInfo[] seasons;

    public String[] getCountry() {
        return country;
    }

    public SeasonMetaInfo[] getSeasons() {
        return seasons;
    }

    public ContentMetaInfo toContentMetaInfo() {
        return new ContentMetaInfo(
                getName(),
                getImages().get("v_banner_3_4").getSizedUrlTemplate().replace("%size%", "135x180"),
                Integer.valueOf(getYear()),
                null,
                getRestriction(),
                getCountry()[0],
                null,
                getDescription()
        );
    }

    public ContentMetaInfo toSeasonContentMetaInfo(String seasonId) {
        SeasonMetaInfo seasonMetaInfo = null;
        for (SeasonMetaInfo info : seasons) {
            if (seasonId.equals(info.getId())) {
                seasonMetaInfo = info;
                break;
            }
        }

        return new ContentMetaInfo(
                getName(),
                getImages().get("v_banner_3_4").getSizedUrlTemplate().replace("%size%", "135x180"),
                seasonMetaInfo != null ? Integer.valueOf(seasonMetaInfo.getYear()) : null,
                null,
                getRestriction(),
                getCountry()[0],
                seasonMetaInfo != null ? seasonMetaInfo.getNumber() : null,
                getDescription()
        );
    }


    public static class SerialMetaInfoDTO {
        private SerialMetaInfo serial;

        public SerialMetaInfo getSerial() {
            return serial;
        }
    }

    public static class SeasonMetaInfo {
        private String id;
        private String year;
        private Integer number;

        public String getId() {
            return id;
        }

        public String getYear() {
            return year;
        }

        public Integer getNumber() {
            return number;
        }
    }
}
