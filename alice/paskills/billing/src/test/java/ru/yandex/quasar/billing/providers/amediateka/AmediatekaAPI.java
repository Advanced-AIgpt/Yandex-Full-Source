package ru.yandex.quasar.billing.providers.amediateka;

import java.net.URI;
import java.time.Duration;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;
import java.util.function.BiFunction;
import java.util.function.Function;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.cache.CacheBuilder;
import com.google.common.cache.CacheLoader;
import com.google.common.cache.LoadingCache;
import com.google.common.collect.ImmutableSet;
import com.google.common.util.concurrent.UncheckedExecutionException;
import org.json.JSONObject;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.web.client.RestTemplateBuilder;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.stereotype.Component;
import org.springframework.web.client.HttpClientErrorException;
import org.springframework.web.client.RestTemplate;
import org.springframework.web.util.UriComponentsBuilder;

import ru.yandex.quasar.billing.beans.ContentMetaInfo;
import ru.yandex.quasar.billing.beans.ContentType;
import ru.yandex.quasar.billing.beans.EpisodeProviderContentItem;
import ru.yandex.quasar.billing.beans.ProviderContentItem;
import ru.yandex.quasar.billing.beans.SeasonProviderContentItem;
import ru.yandex.quasar.billing.config.AmediatekaConfig;
import ru.yandex.quasar.billing.config.BillingConfig;
import ru.yandex.quasar.billing.config.SecretsConfig;
import ru.yandex.quasar.billing.exception.BadRequestException;
import ru.yandex.quasar.billing.exception.NotFoundException;
import ru.yandex.quasar.billing.exception.ProviderUnauthorizedException;
import ru.yandex.quasar.billing.exception.UpstreamFailureException;
import ru.yandex.quasar.billing.providers.amediateka.model.Bundle;
import ru.yandex.quasar.billing.providers.amediateka.model.BundleItem;
import ru.yandex.quasar.billing.providers.amediateka.model.Episode;
import ru.yandex.quasar.billing.providers.amediateka.model.FilmMetaInfo;
import ru.yandex.quasar.billing.providers.amediateka.model.MultipleDTO;
import ru.yandex.quasar.billing.providers.amediateka.model.PromoCode;
import ru.yandex.quasar.billing.providers.amediateka.model.Season;
import ru.yandex.quasar.billing.providers.amediateka.model.SeasonEpisodes;
import ru.yandex.quasar.billing.providers.amediateka.model.Serial;
import ru.yandex.quasar.billing.providers.amediateka.model.SerialMetaInfo;
import ru.yandex.quasar.billing.providers.amediateka.model.Stream;
import ru.yandex.quasar.billing.providers.amediateka.model.Subscription;

/**
 * A Component to hold all explicit API interactions to Amediateka and helpers for them
 */
@Component
class AmediatekaAPI {

    /**
     * All codes:
     * missing_client: Клиент не был передан.
     * invalid_customer_credentials: Неверный логин или пароль.
     * invalid_client_credentials: Доступ клиента API запрещён
     * customer_not_confirmed: Пользователь не подтверждён.
     * credential_not_confirmed: Данный способ входа не подтвеждён.
     * client_not_match: Токен был выдан для другого клиента.
     * invalid_token: Неверный токен. Повторите процесс авторизации для получения нового токена.
     * invalid_ip: Доступ с данного IP адреса запрещён.
     * invalid_secret: Неверный ключ.
     * secret_revoked: Данная версия больше не поддерживается. Пожалуйста, обновите приложение.
     * device_not_registered: Устройство не зарегистрировано.
     * invalid_device_credentials: Устройство не зарегистрировано.
     * insufficient_privileges: Недостаточно прав для выполнения данного запроса.
     * not_allowed_to_remove_device: Вы можете удалять устройство не чаще одного раза в месяц.
     * not_allowed_to_attach_device: Вы не можете привязать это устройство.
     * too_many_devices: Невозможно добавить более 5 устройств.
     * too_many_requests: Превышено количество запросов
     * missing_device: Устройство не определено для указанного токена.
     * invalid_token_schema: Неверный формат схемы JWT токена
     * restricted_by_geo: Сервис не доступен в вашей стране.
     */
    private static final Set<String> INVALID_SESSION_ERROR_TYPES = ImmutableSet.of(
            "invalid_token",
            "client_not_match",
            "customer_not_confirmed",
            "credential_not_confirmed");
    private final AmediatekaConfig config;
    private final SecretsConfig secretsConfig;
    private final RestTemplate restTemplate;
    private final LoadingCache<String, List<Episode>> episodesCache;
    private final LoadingCache<String, List<Season>> seasonsCache;

    @Autowired
    AmediatekaAPI(BillingConfig billingConfig, SecretsConfig secretsConfig, RestTemplateBuilder restTemplatebuilder) {
        this.config = billingConfig.getAmediatekaConfig();
        this.secretsConfig = secretsConfig;
        this.restTemplate = restTemplatebuilder
                .setConnectTimeout(Duration.ofSeconds(3L))
                .setReadTimeout(Duration.ofSeconds(3L))
                .build();

        this.episodesCache = CacheBuilder.newBuilder()
                .expireAfterWrite(10, TimeUnit.MINUTES)
                .build(new CacheLoader<String, List<Episode>>() {
                    @Override
                    public List<Episode> load(@Nonnull String seasonId) {
                        return AmediatekaAPI.this.restTemplate.getForObject(
                                externalUri().path("seasons", seasonId, "episodes").build(),
                                SeasonEpisodes.class
                        ).getEpisodes();
                    }
                });

        this.seasonsCache = CacheBuilder.newBuilder()
                .expireAfterWrite(10, TimeUnit.MINUTES)
                .build(new CacheLoader<String, List<Season>>() {
                    @Override
                    public List<Season> load(@Nonnull String serialId) {
                        return AmediatekaAPI.this.restTemplate.getForObject(
                                externalUri().path("serials", serialId).build(),
                                Serial.SerialDTO.class
                        ).getSerial()
                                .getSeasons();
                    }
                });
    }

    /**
     * Retrieves all the objects from paginated url as a Stream
     *
     * @param urlParts base url parts
     * @param dtoClass a MultipleDTO for resulting object type.
     * @return a Stream for all the objects, in order.
     */
    /*package*/
    private <T extends MultipleDTO<V>, V> Collection<V> getAll(Class<T> dtoClass, String... urlParts) {


        T body = this.restTemplate.getForEntity(this.uri().path(urlParts).build(), dtoClass).getBody();
        Collection<V> parts = new ArrayList<>(body.getPayload());

        while (!body.getMeta().getPagination().isLastPage()) {
            body = this.restTemplate.getForObject(
                    this.uri().path(urlParts).offset(body.getMeta().getPagination().nextOffset()).build(),
                    dtoClass
            );

            parts.addAll(body.getPayload());
        }

        return parts;
    }

    /**
     * Retrieve all bundles from amediateka
     *
     * @return collection of bundles
     */
    Collection<Bundle> getAllBundles() {
        Collection<Bundle> bundles = getAll(Bundle.BundlesDTO.class, "bundles");

        return bundles.stream()
                .filter(bundle -> bundle.getOffers().stream().anyMatch(offer -> !offer.getPrices().isEmpty()))
                .collect(Collectors.toList());
    }

    /**
     * Retrieve all items if the specified bundle
     *
     * @param bundle bundle which items to be retrieved
     * @return collection of bundle's items
     */
    Collection<? extends BundleItem> getBundleItems(Bundle bundle) {
        return getAll(BundleItem.BundleItemsDTO.class, "bundles", bundle.getId(), "items");
    }

    @Nullable
    String getStreamUrl(ProviderContentItem contentItem, String session, String userIp) {
        if (session == null) {
            return null;  // no session -- cant query api
        }
        AmediatekaType amediatekaType = AmediatekaType.forContentType(contentItem.getContentType());

        String streamableContentId = amediatekaType.getLeafContentId(this, contentItem);

        List<Stream> streams;
        try {
            streams = this.restTemplate.getForObject(
                    this.uri()
                            .path(amediatekaType.getStreamableAmediatekaType().urlPrefix, streamableContentId,
                                    "streams")
                            .session(session)
                            .param("customer_ip", userIp)
                            .param("platform", "desktop")
                            .param("drm", "widevine")
                            .param("protocol", "dash")
                            .build(),
                    Stream.StreamsDTO.class
            ).getStreams();

        } catch (HttpClientErrorException e) {
            throw refineException(e);
        }
        return streams.stream()
                .findFirst()
                .map(Stream::getUrl)
                .orElseThrow(() -> new NotFoundException("Amediateka stream url not found"));
    }

    /**
     * Check if specified content item is available for the user.
     * <p>
     * Availability check is only possible for leaf entities (i.e. for episodes but not for seasons)
     * so for Serials / Seasons we get second (first is likely to be free) episode's one.
     * <p>
     *
     * @param contentItem content item to check
     * @param session     user access token
     * @return if content is available
     * @throws ProviderUnauthorizedException if bad session is provided
     */
    boolean isContentAvailable(ProviderContentItem contentItem, String session) {
        if (session == null) {
            return false;  // no session -- cant query api
        }

        AmediatekaType amediatekaType = AmediatekaType.forContentType(contentItem.getContentType());
        String leafContentId = amediatekaType.getLeafContentId(this, contentItem);

        if (leafContentId == null) {
            return false;  // no lead content object for this content
        }

        try {
            ResponseEntity<String> result = this.restTemplate.getForEntity(
                    this.uri()
                            .path(amediatekaType.getStreamableAmediatekaType().urlPrefix, leafContentId, "availability")
                            .session(session)
                            .param("platform", "desktop")
                            .param("customer_ip", "5.255.255.5")
                            .build(),
                    String.class
            );
            return result.getStatusCode() == HttpStatus.NO_CONTENT;
        } catch (HttpClientErrorException e) {
            RuntimeException refinedException = refineException(e);
            if (refinedException instanceof ProviderUnauthorizedException) {
                throw refinedException;
            } else {
                return false;
            }

        }
    }

    // TODO: make public / package when needed
    private List<Episode> getSeasonEpisodes(String seasonId) throws ExecutionException {
        return this.episodesCache.get(seasonId);
    }

    private List<Season> getSerialSeasons(String serialId) throws ExecutionException {
        return this.seasonsCache.get(serialId);
    }

    /**
     * takes seasons's second episode id if it is there, of first one, or null
     */
    @Nullable
    private String getSeasonRelevantEpisodeId(String seasonId) {
        try {
            return this.getSeasonEpisodes(seasonId).stream()
                    .limit(2)
                    .reduce((a, b) -> b)
                    .map(Episode::getId)
                    .orElse(null);
        } catch (ExecutionException | UncheckedExecutionException e) {
            return null; //  We failed to access episodes url -- maybe there is no such season
        }
    }

    /**
     * takes serial's second season's relevant episode id if it is there, of first one's, or null
     */
    @Nullable
    private String getSerialAvailabilityRelevantEpisodeId(String serialId) {
        try {
            return this.getSerialSeasons(serialId).stream()
                    .limit(2)
                    .reduce((a, b) -> b)
                    .map(Season::getId)
                    .map(this::getSeasonRelevantEpisodeId)
                    .orElse(null);
        } catch (ExecutionException | UncheckedExecutionException e) {
            return null; //  We failed to access serial url -- maybe there is no such serial
        }
    }

    void payExternal(String priceId, String bundleId, String session) {
        ResponseEntity<String> entity;
        try {
            entity = restTemplate.postForEntity(
                    externalUri().path("pay", "external_payment")
                            .session(session)
                            .param("price_id", priceId)
                            .param("bundle_id", bundleId)
                            .build(),
                    null,
                    String.class
            );
        } catch (HttpClientErrorException e) {
            throw refineException(e);
        }

        if (entity.getStatusCode() != HttpStatus.NO_CONTENT) {
            throw new UpstreamFailureException("Unexpected response status code " + entity.getStatusCode());
        }
    }

    private FilmMetaInfo getFilmMetaInfo(String filmId) {
        return restTemplate.getForObject(
                externalUri().path("films", filmId).build(),
                FilmMetaInfo.FilmMetaInfoDTO.class
        ).getFilm();
    }

    private SerialMetaInfo getSerialMetaInfo(String serialId) {
        return restTemplate.getForObject(
                externalUri().path("serials", serialId).build(),
                SerialMetaInfo.SerialMetaInfoDTO.class
        ).getSerial();
    }

    private Bundle getBundleMetaInfo(String bundleId) {
        return restTemplate.getForObject(
                uri().path("bundles", bundleId).build(),
                Bundle.BundleDTO.class
        ).getBundle();
    }

    private ContentMetaInfo getSeasonContentMetaInfo(String serialId, String seasonId) {
        return getSerialMetaInfo(serialId).toSeasonContentMetaInfo(seasonId);
    }

    PromoCode activatePromoCode(String promoCode, String session) {
        return restTemplate.postForEntity(
                externalUri().path("pay", "promo_code", promoCode)
                        .session(session)
                        .build(),
                null,
                PromoCode.PromoCodeDTO.class
        ).getBody().getPromoCode();
    }

    @Nonnull
    List<Subscription> getActiveSubscriptionsState(String session) {
        try {
            Subscription.SubscriptionsDTO subscriptionsDTO = restTemplate.getForObject(
                    externalUri()
                            .path("user", "subscriptions")
                            .session(session)
                            .build(),
                    Subscription.SubscriptionsDTO.class
            );

            if (subscriptionsDTO != null) {
                // if there is only one subscription amediateka currently returns it in the main subscription block
                // and doesn't return subscriptions array at all
                if (subscriptionsDTO.getSubscriptions() != null) {
                    return subscriptionsDTO.getSubscriptions();
                } else if (subscriptionsDTO.getSubscription() != null) {
                    return Collections.singletonList(subscriptionsDTO.getSubscription());
                }
            }
        } catch (HttpClientErrorException e) {
            throw refineException(e);
        }

        return Collections.emptyList();
    }

    /**
     * Метод анализа ошибки, чтобы перехватить информацию о том, что сессия не валидна
     * Амедиатека возвращает 403 код в случае с проблемами с сессией, вот примеры боди ошибок с 403 кодом:
     * {"meta":{"error_type":"client_not_match","error_message":"Токен был выдан для другого клиента.","status":"403"}}
     * {"meta":{"error_type":"invalid_secret","error_message":"Неверный ключ.","status":"403"}}
     */
    private RuntimeException refineException(HttpClientErrorException original) {
        RuntimeException refinedException = original;
        // Amediateka uses 403 exception for invalid session cases and for other forbidden operations
        if (original.getStatusCode() == HttpStatus.FORBIDDEN) {

            try {
                JSONObject body = new JSONObject(original.getResponseBodyAsString());

                // amediateka used to store error type inside meta
                // but recently we spotted it places into "id" field in the root of response body object
                if ((body.has("meta") && INVALID_SESSION_ERROR_TYPES.contains(body.getJSONObject("meta").optString(
                        "error_type"))) ||
                        (body.has("id") && INVALID_SESSION_ERROR_TYPES.contains(body.optString("id")))) {
                    refinedException = new ProviderUnauthorizedException(original.getResponseBodyAsString(),
                            AmediatekaContentProvider.PROVIDER_NAME, true, original);
                }
            } catch (Exception ignored) {
                // if any unexpected exception happened - reraise initial one
            }

        } else if (original.getStatusCode() == HttpStatus.UNAUTHORIZED) {
            // If they say we have to authorize, make BASS ask user to relogin.
            // According to doc Amediateka throws such exceptions
            refinedException = new ProviderUnauthorizedException(AmediatekaContentProvider.PROVIDER_NAME, true,
                    original);
        }
        return refinedException;
    }

    private AmediatekaURIBuilder uri() {
        return new AmediatekaURIBuilder(false);
    }

    private AmediatekaURIBuilder externalUri() {
        return new AmediatekaURIBuilder(true);
    }

    enum AmediatekaType {
        FILM(
                ContentType.MOVIE,
                "films",
                (api, contentId) -> contentId,
                null,
                (api, providerContentItem) -> api.getFilmMetaInfo(providerContentItem.getId()).toContentMetaInfo(),
                (api, providerContentItem) -> providerContentItem
        ),
        EPISODE(
                ContentType.EPISODE,  // FIXME should use different CONTENT_TYPE maybe?
                "episodes",
                (api, contentId) -> contentId,
                null,
                (api, providerContentItem) -> api.getSeasonContentMetaInfo(
                        ((EpisodeProviderContentItem) providerContentItem).getTvShowId(),
                        ((EpisodeProviderContentItem) providerContentItem).getSeasonId()
                ),
                (api, providerContentItem) -> ((EpisodeProviderContentItem) providerContentItem).toSeason()
        ),
        SEASON(
                ContentType.SEASON,
                "seasons",
                AmediatekaAPI::getSeasonRelevantEpisodeId,
                EPISODE,
                (api, providerContentItem) -> api.getSeasonContentMetaInfo(
                        ((SeasonProviderContentItem) providerContentItem).getTvShowId(),
                        providerContentItem.getId()
                ),
                (api, providerContentItem) -> providerContentItem
        ),
        SERIAL(
                ContentType.TV_SHOW,  // FIXME should use different CONTENT_TYPE maybe?
                "serials",
                AmediatekaAPI::getSerialAvailabilityRelevantEpisodeId,
                EPISODE,
                (api, providerContentItem) -> api.getSerialMetaInfo(providerContentItem.getId()).toContentMetaInfo(),
                (api, providerContentItem) -> providerContentItem
        ),
        BUNDLE(
                ContentType.SUBSCRIPTION,
                "bundles",
                (api, contentId) -> contentId,
                null,
                (api, providerContentItem) -> api.getBundleMetaInfo(providerContentItem.getId()).toContentMetaInfo(),
                (api, providerContentItem) -> providerContentItem
        );

        private static final Map<ContentType, AmediatekaType> OBJECTS_FOR_CONTENT_TYPE =
                Collections.unmodifiableMap(initMap());
        private final ContentType contentType;
        private final String urlPrefix;
        private final BiFunction<AmediatekaAPI, String, String> streamableIdFinderFactory;
        private final AmediatekaType streamableAmediatekaType;
        private final BiFunction<AmediatekaAPI, ProviderContentItem, ContentMetaInfo> contentMetaInfoFactory;
        private final BiFunction<AmediatekaAPI, ProviderContentItem, ProviderContentItem> purchaseableIdFinderFactory;

        AmediatekaType(
                ContentType contentType,
                String urlPrefix,
                BiFunction<AmediatekaAPI,
                        String, String> streamableIdFinderFactory,
                AmediatekaType streamableAmediatekaType,
                BiFunction<AmediatekaAPI, ProviderContentItem, ContentMetaInfo> contentMetaInfoFactory,
                BiFunction<AmediatekaAPI, ProviderContentItem, ProviderContentItem> purchaseableIdFinderFactory
        ) {
            this.contentType = contentType;
            this.urlPrefix = urlPrefix;
            this.streamableIdFinderFactory = streamableIdFinderFactory;
            this.streamableAmediatekaType = streamableAmediatekaType;
            this.contentMetaInfoFactory = contentMetaInfoFactory;
            this.purchaseableIdFinderFactory = purchaseableIdFinderFactory;
        }

        @Nonnull
        public static AmediatekaType forContentType(ContentType contentType) {
            AmediatekaType result = OBJECTS_FOR_CONTENT_TYPE.get(contentType);

            if (result == null) {
                throw BadRequestException.unsupportedContentType(contentType);
            }

            return result;
        }

        private static Map<ContentType, AmediatekaType> initMap() {
            return Arrays.stream(AmediatekaType.values()).collect(Collectors.toMap(
                    AmediatekaType::getContentType,
                    Function.identity()
            ));
        }

        public ContentType getContentType() {
            return contentType;
        }

        public String getUrlPrefix() {
            return urlPrefix;
        }

        public AmediatekaType getStreamableAmediatekaType() {
            return streamableAmediatekaType != null ? streamableAmediatekaType : this;
        }

        public String getLeafContentId(AmediatekaAPI api, ProviderContentItem contentItem) {
            return streamableIdFinderFactory.apply(api, contentItem.getId());
        }

        public ContentMetaInfo getContentMetaInfo(AmediatekaAPI api, ProviderContentItem contentItem) {
            return contentMetaInfoFactory.apply(api, contentItem);
        }

        public ProviderContentItem getPurchasingContentItem(AmediatekaAPI api, ProviderContentItem contentItem) {
            return purchaseableIdFinderFactory.apply(api, contentItem);
        }
    }

    private class AmediatekaURIBuilder {
        private final UriComponentsBuilder uriComponentsBuilder;

        private AmediatekaURIBuilder(boolean external) {
            uriComponentsBuilder = UriComponentsBuilder
                    .fromUriString(AmediatekaAPI.this.config.getApiUrl())
                    .queryParam("client_id", AmediatekaAPI.this.config.getClientId())
                    .queryParam("client_secret", AmediatekaAPI.this.secretsConfig.getAmediatekaClientSecret());

            if (external) {
                uriComponentsBuilder.pathSegment("external");
            }

            uriComponentsBuilder.pathSegment("v1");
        }

        private AmediatekaURIBuilder path(String... parts) {
            String[] firstParts = Arrays.copyOf(parts, parts.length - 1);
            String lastPart = parts[parts.length - 1];

            uriComponentsBuilder
                    .pathSegment(firstParts)
                    .path(lastPart + ".json");
            return this;
        }

        private AmediatekaURIBuilder limit(int limit) {
            return param("limit", Integer.toString(limit));
        }

        private AmediatekaURIBuilder offset(int offset) {
            return param("offset", Integer.toString(offset));
        }

        private AmediatekaURIBuilder session(String session) {
            return this.param("access_token", session);
        }

        private AmediatekaURIBuilder param(String name, String value) {
            uriComponentsBuilder.queryParam(name, value);
            return this;
        }

        private URI build() {
            return uriComponentsBuilder.build().encode().toUri();
        }
    }
}
