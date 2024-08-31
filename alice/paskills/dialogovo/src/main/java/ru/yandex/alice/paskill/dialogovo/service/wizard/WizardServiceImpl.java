package ru.yandex.alice.paskill.dialogovo.service.wizard;

import java.net.URI;
import java.time.Duration;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.CompletableFuture;
import java.util.stream.Collectors;

import com.fasterxml.jackson.core.JsonParseException;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.JsonMappingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.common.cache.CacheBuilder;
import com.google.common.cache.CacheLoader;
import com.google.common.cache.CacheStats;
import com.google.common.cache.LoadingCache;
import com.google.common.util.concurrent.UncheckedExecutionException;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.web.client.HttpClientErrorException;
import org.springframework.web.client.RestTemplate;
import org.springframework.web.util.UriComponentsBuilder;

import ru.yandex.alice.paskill.dialogovo.config.WizardConfig;
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.CustomEntity;
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.Intent;
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.NluEntity;
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.StringEntity;
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.UnknownBuiltinEntity;
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.json.NluEntityDeserializer;
import ru.yandex.alice.paskill.dialogovo.utils.executor.DialogovoInstrumentedExecutorService;

import static java.util.Objects.requireNonNullElse;
import static ru.yandex.alice.paskill.dialogovo.service.wizard.WizardServiceConfiguration.RETRY_COUNT;
import static ru.yandex.alice.paskill.dialogovo.utils.executor.DialogovoInstrumentedExecutorService.TIMEOUT_DELTA;

class WizardServiceImpl implements WizardService {
    private static final Logger logger = LogManager.getLogger();
    // wizard query params
    private static final String WIZARD_RWR_LIST = String.join(",", List.of(
            // required rules
            "Granet",
            "GranetCompiler",
            "GranetConfig",
            "GranetStaticConfig",
            "AliceEmbeddings",
            "AliceNonsenseTagger",
            "AliceNormalizer",
            "AliceRequest",
            "AliceSampleFeatures",
            "AliceTokenEmbedder",
            "ExternalMarkup",
            // entities for ner and granet slots
            "Date",
            "Fio",
            "GeoAddr",
            "FstFloat",
            "FstNum",
            "AliceUserEntities",
            // optional AliceTagger dependencies
            "CustomEntities",
            "DirtyLang",
            "EntityFinder",
            "FstAlbum",
            "FstArtist",
            "FstCurrency",
            "FstFilms100_750",
            "FstFilms50Filtered",
            "FstPoiCategoryRu",
            "FstSite",
            "FstSoft",
            "FstSwear",
            "FstTrack",
            "FstCalc",
            "FstDate",
            "FstDatetime",
            "FstDatetimeRange",
            "FstFio",
            "FstFloat",
            "FstGeo",
            "FstNum",
            "FstTime",
            "FstUnitsTime",
            "FstWeekdays",
            "IsNav"
    ));
    // https://yql.yandex-team.ru/Operations/XnuSL2HljkdueRJk2zN-GwM0o40EVznUvtneCvdIQDQ=
    private static final Set<String> INITIAL_CACHE = Set.of(
            "Извините, я вас не поняла.",
            "Извините, диалог не отвечает",
            "Извините, навык не отвечает",
            "Не понимаю команду. Подсказка, для продолжения вам нужно ответить \"Да\", \"Нет\", \"Не знаю\", " +
                    "\"Возможно\", \"Скорее нет\", \"Назад\". Для вызова помощи скажите команду \"Помощь\".",
            "Извините, я задумалась. Повторите, пожалуйста.",
            "Ой, что-то я призадумалась. Пожалуйста, повторите.",
            "Не понимаю команду. Подсказка, для продолжения вам нужно ответить \"Да\". " +
                    "Для вызова помощи скажите команду \"Помощь\".",
            "Извините, я не поняла.",
            "Не смог понять вашу команду. Попробуйте сказать команду еще раз.",
            "Простите но я не поняла Вашу команду. Попробуйте снова.",
            "Я вас не понимаю. Воспользуйтесь помощью!"
    );
    private final RestTemplate webClient;
    private final DialogovoInstrumentedExecutorService executor;
    private final WizardConfig wizardConfig;
    private final ObjectMapper objectMapper;
    private final LoadingCache<String, String> cache;

    WizardServiceImpl(WizardConfig wizardConfig,
                      RestTemplate restTemplate,
                      DialogovoInstrumentedExecutorService executor,
                      ObjectMapper objectMapper,
                      boolean firstCacheWarmUpOccurred
    ) {
        this.wizardConfig = wizardConfig;
        this.webClient = restTemplate;
        this.executor = executor;
        this.objectMapper = objectMapper;
        this.cache = CacheBuilder.newBuilder()
                .expireAfterWrite(Duration.ofSeconds(wizardConfig.getCacheTtlSeconds()))
                .maximumSize(wizardConfig.getCacheSize())
                .recordStats()
                .build(new CacheLoader<String, String>() {
                    @Override
                    public String load(String key) {
                        return callWizard(key, Optional.empty());
                    }
                });

        if (firstCacheWarmUpOccurred) {
            initCache();
        }
    }

    CacheStats getCacheStats() {
        return this.cache.stats();
    }

    private void initCache() {
        for (String s : INITIAL_CACHE) {
            try {
                cache.getUnchecked(s);
            } catch (RuntimeException e) {
                logger.error("unable to preload wizard cache", e);
            }
        }
    }

    @Override
    public WizardResponse requestWizard(Optional<String> utterance, Optional<String> grammarsBase64) {
        if (utterance.isEmpty() || "".equals(utterance.get().trim())) {
            logger.info("Not sending request to wizard, utterance is empty");
            return WizardResponse.EMPTY;
        }
        logger.info("Grammar: {}", grammarsBase64.orElse(""));
        String responseStr = null;
        try {
            responseStr = grammarsBase64
                    .map(grammar -> callWizard(utterance.get(), grammarsBase64))
                    // use cache for only requests without grammar so that cache stale doesn't affect wizard
                    .orElseGet(() -> {
                        try {
                            return cache.getUnchecked(utterance.get());
                        } catch (UncheckedExecutionException e) {
                            if (e.getCause() == null) {
                                throw e;
                            } else if (e.getCause() instanceof RuntimeException) {
                                throw (RuntimeException) e.getCause();
                            } else {
                                throw new RuntimeException(e.getCause());
                            }
                        }
                    });

            WizardHttpResponse response = objectMapper.readValue(responseStr, WizardHttpResponse.class);

            if (response == null) {
                logger.error("Failed to parse wizard response");
                return WizardResponse.EMPTY;
            }
            return new WizardResponse(parseHttpResponse(response));
        } catch (JsonParseException | JsonMappingException e) {
            logger.error("Failed to parse wizard response: " + responseStr + "utterance=[" + utterance.get() + "]", e);
            return WizardResponse.EMPTY;
        } catch (HttpClientErrorException e) {
            logger.error("Unable to request wizard: utterance=" + utterance + ", responseBody = " +
                    e.getResponseBodyAsString(), e);
            return WizardResponse.EMPTY;
        } catch (Exception e) {
            logger.error("Unable to request wizard: utterance={}", utterance, e);
            return WizardResponse.EMPTY;
        }
    }

    private String callWizard(String utterance, Optional<String> grammarsBase64) {
        StringBuilder wizextra = new StringBuilder("alice_preprocessing=true;bg_paskills");
        // grammarsBase64 already contains bg_granet_source_text prefix
        grammarsBase64.ifPresent(s -> wizextra.append(";").append(s));

        var url = UriComponentsBuilder.newInstance()
                .uri(URI.create(wizardConfig.getUrl()))
                .queryParam("format", "json")
                .queryParam("tld", "ru")
                .queryParam("uil", "ru")
                .queryParam("rwr", WIZARD_RWR_LIST)
                .queryParam("wizclient", "dialogovo")
                .queryParam("wizextra", wizextra)
                .queryParam("text", utterance)
                .build();
        logger.info("Wizard request");
        logger.debug("Wizard request url: {}", url);
        String responseStr = requireNonNullElse(webClient.getForObject(url.toString(), String.class), "");
        logger.debug("Response body: {}", responseStr);
        return responseStr;
    }

    @Override
    public CompletableFuture<WizardResponse> requestWizardAsync(Optional<String> utterance,
                                                                Optional<String> grammarsBase64) {
        return executor.supplyAsyncInstrumented(
                () -> requestWizard(utterance, grammarsBase64),
                Duration.ofMillis(RETRY_COUNT * wizardConfig.getTimeout() + TIMEOUT_DELTA)
        );
    }

    private Map<String, Intent> parseHttpResponse(WizardHttpResponse httpResponse) {
        Map<String, Intent> intents = new HashMap<>();
        for (var form : httpResponse.getRules().getGranet().getForms()) {
            Map<String, NluEntity> slots = new HashMap<>();
            final var tags = new ArrayList<>(form.getTags());
            Collections.sort(tags);
            for (final var tag : tags) {
                Optional<NluEntity> slot = makeSlot(tag, httpResponse.getRules().getGranet().getTokens());
                if (slot.isPresent()) {
                    final NluEntity nluEntity = slot.get();
                    final boolean overlapsWithExistingSlot = slots.values().stream()
                            .anyMatch(s -> s.getTokens().equals(nluEntity.getTokens()) &&
                                    s.getType().equals(nluEntity.getType()));
                    if (!overlapsWithExistingSlot) {
                        if (!slots.containsKey(tag.getName())) {
                            slots.put(tag.getName(), nluEntity);
                        } else {
                            slots.get(tag.getName()).merge(nluEntity);
                        }
                    }
                }
            }
            intents.put(form.getName(), new Intent(form.getName(), slots));
        }
        return intents;
    }

    private Optional<NluEntity> makeSlot(
            WizardHttpResponse.Tag tag,
            List<WizardHttpResponse.Token> tokens
    ) {
        final List<WizardHttpResponse.TagData> dataList = tag.getData();
        final Optional<NluEntity> slot;
        if (dataList.size() > 0 && dataList.get(0).getType() != null && dataList.get(0).getValue() != null) {
            WizardHttpResponse.TagData data = dataList.get(0);
            // null warnings are already checked in the `if` previous statement ^^
            if (data.getType().startsWith("YANDEX.")) {
                try {
                    NluEntity deserialized = NluEntityDeserializer.entityFromString(
                            data.getType(),
                            data.getBegin(),
                            data.getEnd(),
                            data.getValue(),
                            objectMapper);
                    slot = deserialized instanceof UnknownBuiltinEntity
                            ? Optional.empty()
                            : Optional.of(deserialized);
                } catch (JsonProcessingException e) {
                    logger.error("Failed to parse slot value from tag {}", tag, e);
                    return Optional.empty();
                }
            } else {
                slot = Optional.of(new CustomEntity(tag.getBegin(), tag.getEnd(), data.getType(), data.getValue()));
            }
        } else {
            String value = tokens
                    .subList(tag.getBegin(), tag.getEnd())
                    .stream()
                    .map(WizardHttpResponse.Token::getText)
                    .collect(Collectors.joining(" "));
            slot = Optional.of(new StringEntity(tag.getBegin(), tag.getEnd(), value));
        }
        return slot;
    }

}
