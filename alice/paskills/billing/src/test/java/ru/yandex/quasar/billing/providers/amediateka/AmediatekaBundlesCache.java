package ru.yandex.quasar.billing.providers.amediateka;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.function.Function;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Component;

import ru.yandex.quasar.billing.beans.ContentType;
import ru.yandex.quasar.billing.beans.ProviderContentItem;
import ru.yandex.quasar.billing.providers.amediateka.model.BaseObject;
import ru.yandex.quasar.billing.providers.amediateka.model.Bundle;
import ru.yandex.quasar.billing.providers.amediateka.model.ProviderContentIterable;

import static java.util.stream.Collectors.groupingBy;
import static java.util.stream.Collectors.toMap;

/**
 * Позволяет по id фильма, сезона, сериала или подписки получить Bundle-е в которые он входит.
 * Под капотом работает простыми id-ками (ProviderContentItem.JustIdProviderContentItem) т.к. для его работы
 * достаточно информации, которая в нём есть.
 */
@Component
class AmediatekaBundlesCache {

    private static final Logger log = LogManager.getLogger();

    private final AmediatekaAPI amediatekaAPI;

    private volatile State state;

    @Autowired
    AmediatekaBundlesCache(AmediatekaAPI amediatekaAPI) {
        this.amediatekaAPI = amediatekaAPI;
    }

    /**
     * This method is called by the Spring in a background thread i.e. asynchronously
     */
    //@Scheduled(fixedDelay = 1000 * 60 * 10, initialDelay = 1000 * 10)
    // refresh each 10 mins -- param is in milliseconds
    private void refresh() {
        log.info("Refreshing Amediateka bundle cache");

        try {
            Collection<Bundle> bundles = this.amediatekaAPI.getAllBundles();

            Map<String, Bundle> bundleByIdMap = bundles.stream().collect(toMap(Bundle::getId, Function.identity()));

            Map<ProviderContentItem, List<Bundle>> newBundlesMap = getBundlesMap(bundles);

            // add bundles itself as subscription items
            Map<ProviderContentItem, List<Bundle>> bundlesSelfItems = bundles.stream()
                    .collect(groupingBy(bundle -> ProviderContentItem.create(ContentType.SUBSCRIPTION,
                            bundle.getId())));
            newBundlesMap.putAll(bundlesSelfItems);

            Map<String, String> slugToBundleIdMap = bundles.stream()
                    .collect(toMap(Bundle::getSlug, BaseObject::getId));

            state = new State(newBundlesMap, bundlesSelfItems.keySet(), slugToBundleIdMap, bundleByIdMap);
            log.info("Done, cache size is {}", state.getBundlesIndex().size());
        } catch (Exception e) {
            log.error("Cache failed to get populated", e);
        }
    }

    List<Bundle> getBundlesFor(ProviderContentItem providerContentItem) {
        if (state == null) {
            refresh();
        }

        return state.getBundlesIndex().getOrDefault(providerContentItem.asJustItContentItem(), Collections.emptyList());
    }

    Optional<String> getBundleId(String slug) {
        if (state == null) {
            refresh();
        }
        return Optional.ofNullable(state.getSlugToBundleIdMap().get(slug));
    }

    Optional<Bundle> getBundleById(String bundleId) {
        if (state == null) {
            refresh();
        }
        return Optional.ofNullable(state.getBundleByIdMap().get(bundleId));
    }

    /**
     * Goes to amediatekaAPI and retrieves builds index
     *
     * @return index `{Item => List of Bundles that have this Item}`
     */
    private Map<ProviderContentItem, List<Bundle>> getBundlesMap(Collection<Bundle> bundles) {

        Map<ProviderContentItem, List<Bundle>> contentItemToBundleMap = new HashMap<>();

        for (Bundle bundle : bundles) {
            amediatekaAPI.getBundleItems(bundle).stream()
                    .map(ProviderContentIterable::getProviderContentItems)
                    .flatMap(Collection::stream)
                    .forEach(contentItem -> contentItemToBundleMap.computeIfAbsent(contentItem,
                                    key -> new ArrayList<>())
                            .add(bundle));
        }

        return contentItemToBundleMap;

    }

    private static class State {
        // index from content items to bundle.
        private final Map<ProviderContentItem, List<Bundle>> bundlesIndex;

        // indexes of bundles
        private final Collection<ProviderContentItem> bundleIds;
        private final Map<String, String> slugToBundleIdMap;
        private final Map<String, Bundle> bundleByIdMap;

        private State(Map<ProviderContentItem, List<Bundle>> bundlesIndex, Collection<ProviderContentItem> bundleIds,
                      Map<String, String> slugToBundleIdMap, Map<String, Bundle> bundleByIdMap) {
            this.bundlesIndex = bundlesIndex;
            this.bundleIds = bundleIds;
            this.slugToBundleIdMap = slugToBundleIdMap;
            this.bundleByIdMap = bundleByIdMap;
        }

        Map<ProviderContentItem, List<Bundle>> getBundlesIndex() {
            return bundlesIndex;
        }

        Collection<ProviderContentItem> getBundleIds() {
            return bundleIds;
        }

        Map<String, String> getSlugToBundleIdMap() {
            return slugToBundleIdMap;
        }

        Map<String, Bundle> getBundleByIdMap() {
            return bundleByIdMap;
        }
    }
}
