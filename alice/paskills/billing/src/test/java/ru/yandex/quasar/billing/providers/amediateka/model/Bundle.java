package ru.yandex.quasar.billing.providers.amediateka.model;

import java.util.List;

import com.fasterxml.jackson.annotation.JsonIgnore;
import lombok.Getter;

import ru.yandex.quasar.billing.beans.ContentMetaInfo;

@Getter
public class Bundle extends BaseObject {
    private String id;
    private String name;
    private String description;
    private String slug;
    private List<Offer> offers;

    public ContentMetaInfo toContentMetaInfo() {
        return ContentMetaInfo.builder(getName())
                .description(getDescription())
                .build();
    }

    @Getter
    public static class Offer {
        private String name;
        private String uid;
        private List<Price> prices;
    }

    @Getter
    public static class BundleDTO extends Metadated {
        private Bundle bundle;
    }

    @Getter
    public static class BundlesDTO extends MultipleDTO<Bundle> {
        private List<Bundle> bundles;

        @Override
        @JsonIgnore
        public List<Bundle> getPayload() {
            return bundles;
        }
    }
}
