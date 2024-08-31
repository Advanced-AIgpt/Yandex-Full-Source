package ru.yandex.alice.paskill.dialogovo.service.penguinary;

import java.time.Duration;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.concurrent.CompletableFuture;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.web.client.RestTemplate;

import ru.yandex.alice.paskill.dialogovo.config.PenguinaryConfig;
import ru.yandex.alice.paskill.dialogovo.service.penguinary.PenguinaryResult.Candidate;
import ru.yandex.alice.paskill.dialogovo.utils.executor.DialogovoInstrumentedExecutorService;

import static ru.yandex.alice.paskill.dialogovo.utils.executor.DialogovoInstrumentedExecutorService.TIMEOUT_DELTA;

/**
 * PASKILLS-4438: [☂] Развитие Пингвинария - описание DSSM
 */
class PenguinaryServiceImpl implements PenguinaryService {
    static final String NODE_ID = "skill_activation";
    private static final Logger logger = LogManager.getLogger();
    private final RestTemplate restTemplate;
    private final DialogovoInstrumentedExecutorService executorService;
    private final PenguinaryConfig config;

    PenguinaryServiceImpl(PenguinaryConfig config, RestTemplate restTemplate,
                          DialogovoInstrumentedExecutorService executorService) {
        this.config = config;
        this.restTemplate = restTemplate;
        this.executorService = executorService;
    }

    @Override
    public PenguinaryResult findSkillsByUtterance(String utterance) {
        var request = new PenguinaryRequest(NODE_ID, utterance);

        logger.debug("Calling penguinary with body: {}", request);
        PenguinaryResponse response = restTemplate.postForObject(config.getUrl() + "get_intents", request,
                PenguinaryResponse.class);
        logger.debug("Penguinary response: {}", response);

        int size = Math.min(response.getDistances().length, response.getIntents().size());
        List<Candidate> candidates = new ArrayList<>(size);
        for (int i = 0; i < size; i++) {
            candidates.add(new Candidate(response.getDistances()[i], response.getIntents().get(i)));
        }
        candidates.sort(Comparator.comparing(Candidate::getDistance));
        return new PenguinaryResult(candidates);
    }

    @Override
    public CompletableFuture<PenguinaryResult> findSkillsByUtteranceAsync(String utterance) {
        return executorService.supplyAsyncInstrumented(() -> findSkillsByUtterance(utterance),
                Duration.ofMillis(config.getTimeout() + TIMEOUT_DELTA));
    }
}
