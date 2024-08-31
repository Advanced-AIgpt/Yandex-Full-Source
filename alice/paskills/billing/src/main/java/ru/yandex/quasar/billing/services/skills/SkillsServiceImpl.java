package ru.yandex.quasar.billing.services.skills;

import java.security.KeyPair;
import java.security.KeyPairGenerator;
import java.security.NoSuchAlgorithmException;
import java.time.Instant;
import java.util.Base64;
import java.util.List;
import java.util.Objects;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.annotation.JsonValue;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ObjectNode;
import com.google.common.util.concurrent.ThreadFactoryBuilder;
import lombok.Data;
import lombok.Getter;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.dao.DuplicateKeyException;
import org.springframework.data.relational.core.conversion.DbActionExecutionException;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.stereotype.Component;
import org.springframework.web.client.HttpClientErrorException;
import org.springframework.web.client.RestTemplate;

import ru.yandex.quasar.billing.dao.PurchaseInfo;
import ru.yandex.quasar.billing.dao.PurchaseOffer;
import ru.yandex.quasar.billing.dao.SkillInfo;
import ru.yandex.quasar.billing.dao.SkillInfoDAO;
import ru.yandex.quasar.billing.dao.SkillMerchant;
import ru.yandex.quasar.billing.exception.ForbiddenException;
import ru.yandex.quasar.billing.exception.NotFoundException;
import ru.yandex.quasar.billing.providers.ProviderPurchaseException;
import ru.yandex.quasar.billing.services.AuthorizationContext;
import ru.yandex.quasar.billing.services.UnistatService;
import ru.yandex.quasar.billing.services.processing.yapay.ServiceMerchantInfo;
import ru.yandex.quasar.billing.services.processing.yapay.TokenNotFound;
import ru.yandex.quasar.billing.services.processing.yapay.YandexPayMerchantRegistry;
import ru.yandex.quasar.billing.util.ParallelHelper;


@Component
public
class SkillsServiceImpl implements SkillsService {
    static final String ENTITY_ID_PREFIX = "alice-ext-skill:";
    private static final String ALGORITHM = "RSA";
    private final SkillInfoDAO skillInfoDAO;
    private final RestTemplate restTemplate;
    private final YandexPayMerchantRegistry merchantRegistry;
    private final ParallelHelper parallelHelper;
    private final ObjectMapper objectMapper;
    private final UnistatService unistatService;

    SkillsServiceImpl(
            SkillInfoDAO skillInfoDAO,
            RestTemplate restTemplate,
            YandexPayMerchantRegistry merchantRegistry,
            @Qualifier("skillServiceExecutorService") ExecutorService executorService,
            AuthorizationContext authorizationContext,
            ObjectMapper objectMapper,
            UnistatService unistatService) {
        this.skillInfoDAO = skillInfoDAO;
        this.restTemplate = restTemplate;
        this.merchantRegistry = merchantRegistry;
        this.unistatService = unistatService;
        this.parallelHelper = new ParallelHelper(executorService, authorizationContext);
        this.objectMapper = objectMapper;
    }

    @Override
    public SkillInfo getSkillById(Long id) {
        return skillInfoDAO.findBySkillId(id)
                .orElseThrow(() -> new NotFoundException("Unknown skill id: " + id));
    }

    @Override
    public SkillInfo getSkillByUuid(String skillUuid) {
        return skillInfoDAO.findBySkillId(skillUuid)
                .orElseThrow(() -> new NotFoundException("Unknown skill uuid: " + skillUuid));
    }

    @Override
    @Deprecated
    public SkillInfo refreshSkillKeys(Long skillInfoId) {

        SkillInfo skill = getSkillById(skillInfoId);
        StringKeyPair keys = generateKeyPair();

        skill.setKeys(keys.getPrivateKey(), keys.getPublicKey());

        return skillInfoDAO.save(skill);
    }

    @Override
    public SkillInfo registerSkill(String skillUuid, long ownerUid, String slug) throws SkillAccessViolationException {
        StringKeyPair keys = generateKeyPair();

        SkillInfo skillInfo = SkillInfo.builder(skillUuid)
                .ownerUid(ownerUid)
                .slug(slug)
                .privateKey(keys.getPrivateKey())
                .publicKey(keys.getPublicKey())
                .build();

        try {
            return skillInfoDAO.save(skillInfo);
        } catch (DbActionExecutionException e) {
            if (e.getCause() instanceof DuplicateKeyException) {
                SkillInfo existingSkill = skillInfoDAO.findBySkillId(skillUuid)
                        .orElseThrow(() -> new RuntimeException("skill info not found and not saved due to a " +
                                "duplicate exception"));

                if (existingSkill.getOwnerUid() != ownerUid) {
                    throw new SkillAccessViolationException();
                }
                return existingSkill;
            } else {
                throw e;
            }
        }
    }

    @Override
    public MerchantInfo requestMerchantAccess(String skillUuid, String token, String description)
            throws BadSkillAccessRequestTokenException {
        SkillInfo skill = getSkillByUuid(skillUuid);


        ServiceMerchantInfo serviceMerchantInfo;
        if (!skill.getMerchants().containsKey(token)) {
            try {
                serviceMerchantInfo = merchantRegistry.requestMerchantAccess(token, ENTITY_ID_PREFIX + skillUuid,
                        description);
            } catch (TokenNotFound e) {
                throw new BadSkillAccessRequestTokenException(e);
            }
            SkillMerchant skillMerchant = SkillMerchant.builder()
                    .serviceMerchantId(serviceMerchantInfo.getServiceMerchantId())
                    .description(serviceMerchantInfo.getDescription())
                    .entityId(serviceMerchantInfo.getEntityId())
                    .token(token)
                    .build();

            skill.addMerchant(skillMerchant);

            skillInfoDAO.save(skill);
        } else {
            SkillMerchant skillMerchant = skill.getMerchants().get(token);
            serviceMerchantInfo = merchantRegistry.merchantInfo(skillMerchant.getServiceMerchantId());
        }
        return MerchantInfo.fromServiceMerchantInfo(token, serviceMerchantInfo);
    }

    @Override
    public MerchantInfo merchantInfo(String skillUuid, String token) {
        SkillInfo skill = getSkillByUuid(skillUuid);
        SkillMerchant skillMerchant = skill.getMerchants().get(token);
        if (skillMerchant == null) {
            throw new ForbiddenException("Wrong merchant token");
        } else {
            return MerchantInfo.fromServiceMerchantInfo(token,
                    merchantRegistry.merchantInfo(skillMerchant.getServiceMerchantId()));
        }

    }

    @Override
    public List<MerchantInfo> getMerchants(String skillUuid) {
        SkillInfo skill = getSkillByUuid(skillUuid);
        return parallelHelper.processParallel(skill.getMerchants().values(),
                item -> MerchantInfo.fromServiceMerchantInfo(item.getToken(),
                        merchantRegistry.merchantInfo(item.getServiceMerchantId())));
    }

    @Override
    public void executeSkillCallback(
            PurchaseOffer purchaseOffer,
            String purchasePayload,
            String purchaseToken,
            String webhookRequest,
            PurchaseEventType eventType
    ) throws ProviderPurchaseException {
        unistatService.incrementStatValue("quasar_billing_payment-skill-callback_calls_dmmm");
        // TODO: use merchant_token + order_id for purchasetoken
        SkillInfo skill = getSkillById(purchaseOffer.getSkillInfoId());
        String callbackUrl = Objects.requireNonNull(purchaseOffer.getSkillCallbackUrl());

        ResponseEntity<PurchaseEventResponse> response;
        try {
            PurchaseEventRequest request = new PurchaseEventRequest(
                    purchaseOffer.getPurchaseRequestId(),
                    purchaseToken,
                    purchaseOffer.getUuid(),
                    Instant.now().toEpochMilli(),
                    skill.getSkillUuid(),
                    purchaseOffer.getUid(),
                    objectMapper.readValue(purchasePayload, ObjectNode.class),
                    objectMapper.readValue(webhookRequest, ObjectNode.class)
            );

            response = restTemplate.postForEntity(
                    callbackUrl,
                    request,
                    PurchaseEventResponse.class
            );
        } catch (Exception e) {
            unistatService.incrementStatValue("quasar_billing_payment-skill-callback_failures_dmmm");
            throw new ProviderPurchaseException(PurchaseInfo.Status.ERROR_UNKNOWN, e);
        }

        PurchaseEventResponse body = response.getBody();
        if (response.getStatusCode() != HttpStatus.OK) {
            unistatService.incrementStatValue("quasar_billing_payment-skill-callback_failures_dmmm");
            throw new HttpClientErrorException(response.getStatusCode());
        } else if (body == null) {
            unistatService.incrementStatValue("quasar_billing_payment-skill-callback_failures_dmmm");
            throw new HttpClientErrorException(response.getStatusCode(), "response bode is null");
        } else if (body.result != PurchaseEventResponse.ResultStatus.OK) {
            unistatService.incrementStatValue("quasar_billing_payment-skill-callback_failures_dmmm");
            throw new ProviderPurchaseException(PurchaseInfo.Status.ERROR_UNKNOWN, body.resultMessage);
        }
    }

    @Deprecated // TODO remove in PASKILLS-5195
    private StringKeyPair generateKeyPair() {
        KeyPairGenerator kpg;
        try {
            kpg = KeyPairGenerator.getInstance(ALGORITHM);
        } catch (NoSuchAlgorithmException e) {
            throw new RuntimeException(e);
        }
        kpg.initialize(2048);
        KeyPair kp = kpg.generateKeyPair();

        Base64.Encoder encoder = Base64.getEncoder();

        String privateKey = encoder.encodeToString(kp.getPrivate().getEncoded());
        String publicKey = encoder.encodeToString(kp.getPublic().getEncoded());

        return new StringKeyPair(privateKey, publicKey);
    }


    @Data
    private static class StringKeyPair {
        private final String privateKey;
        private final String publicKey;
    }

    @Data
    public static class PurchaseEventRequest {
        @JsonProperty("purchase_request_id")
        private final String purchaseRequestId;

        @JsonProperty("purchase_token")
        private final String purchaseToken;

        @JsonProperty("purchase_offer_uuid")
        private final String purchaseOfferUuid;

        @JsonProperty("purchase_timestamp")
        private final long purchaseTimestamp;

        @JsonProperty("skill_id")
        private final String skillId;

        @JsonProperty("user_id")
        private final String userId;

        @JsonProperty("purchase_payload")
        private final ObjectNode purchasePayload;

        @JsonProperty("webhook_request")
        private final ObjectNode webhookRequest;

        @JsonProperty("version")
        private final String version = "1.0";
    }

    @Data
    public static class PurchaseEventResponse {
        @JsonProperty("result")
        private final ResultStatus result;

        @JsonProperty("result_message")
        private final String resultMessage;

        public enum ResultStatus {
            STATUS_NOT_DEFINED("status_not_defined"),
            OK("ok"),
            ERROR("error");

            @Getter
            @JsonValue
            private final String value;

            ResultStatus(String value) {
                this.value = value;
            }
        }
    }

    @Configuration
    static class ExecutorServiceConfig {
        @Bean(value = "skillServiceExecutorService", destroyMethod = "shutdownNow")
        public ExecutorService supExecutorService() {
            return Executors.newCachedThreadPool(
                    new ThreadFactoryBuilder()
                            .setNameFormat("skill-service-%d")
                            .build()
            );
        }
    }
}
