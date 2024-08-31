package ru.yandex.quasar.billing.services.processing.yapay;

import java.math.BigDecimal;
import java.time.Instant;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.atomic.AtomicLong;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.boot.web.client.RestTemplateBuilder;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpHeaders;
import org.springframework.web.client.RestTemplate;

import ru.yandex.quasar.billing.controller.SimpleResult;
import ru.yandex.quasar.billing.controller.SkillPurchaseCallbackRequest;
import ru.yandex.quasar.billing.exception.NotFoundException;
import ru.yandex.quasar.billing.services.TestAuthorizationService;
import ru.yandex.quasar.billing.services.tvm.TestTvmClientImpl;
import ru.yandex.quasar.billing.services.tvm.TvmClientName;
import ru.yandex.quasar.billing.services.tvm.TvmHeaders;

import static ru.yandex.quasar.billing.controller.SkillPurchaseCallbackRequest.CallbackType.ORDER_STATUS_UPDATED;

public class TestYandexPayClient implements YandexPayClient, YandexPayMerchantTestUtil {

    public static final Long TEST_MERCHANT_ID = 200L;
    private static final Logger logger = LogManager.getLogger();
    private static final ServiceMerchantInfo TEST_MERCHANT_INFO = ServiceMerchantInfo.builder()
            .serviceMerchantId(TEST_MERCHANT_ID)
            .deleted(false)
            .enabled(true)
            .entityId("1234")
            .description("Test")
            .legalAddress("Москва, улица Льва Толстого 16")
            .organization(Organization.builder()
                    .inn("5043041353")
                    .name("Yandex")
                    .englishName("HH")
                    .ogrn("1234567890")
                    .type("OOO")
                    .kpp("504301001")
                    .fullName("Hoofs & Horns")
                    .siteUrl("pay.yandex.ru")
                    .scheduleText("с 9 до 6")
                    .build())
            .build();
    private final ConcurrentMap<Long, List<OrderRecord>> merchantOrders = new ConcurrentHashMap<>();
    private final AtomicLong orderSequence = new AtomicLong();
    private final RestTemplate restTemplate;
    private final BlockingQueue<Runnable> queue = new ArrayBlockingQueue<>(100);
    private final List<Future> tasks = new ArrayList<>();
    private volatile int port;
    //public final static String TEST_MERCHANT_TOKEN = "TEST_MERCHANT_TOKEN!";
    private boolean accessRequested = false;
    private ExecutorService executor;

    public TestYandexPayClient(RestTemplateBuilder restTemplateBuilder) {
        this.restTemplate = restTemplateBuilder.build();
    }

    public void init(int port) {
        this.port = port;
        this.executor = Executors.newSingleThreadExecutor();
    }

    @Override
    public YaPayMerchant getMerchantByKey(String key) {
        if (TEST_MERCHANT_KEY.equals(key)) {
            return MERCHANT;
        } else {
            throw new NotFoundException("Merchant not found for key: " + key);
        }
    }

    @Override
    public Order createOrder(long serviceMerchantId, CreateOrderRequest newOrder) {
        if (serviceMerchantId == TEST_MERCHANT_ID) {
            OrderRecord order = OrderRecord.builder()
                    .serviceMerchantId(serviceMerchantId)
                    .active(true)
                    .items(newOrder.getItems().stream()
                            .map(item -> OrderItem.builder()
                                    .name(item.getName())
                                    .amount(item.getAmount())
                                    .currency(item.getCurrency())
                                    .nds(item.getNds())
                                    .price(item.getPrice())
                                    .productId(1L)
                                    .build())
                            .collect(Collectors.toList()))
                    .orderId(orderSequence.incrementAndGet())
                    .verified(false)
                    .userEmail(newOrder.getUserEmail())
                    .price(newOrder.getItems().stream()
                            .map(item -> item.getPrice().multiply(item.getAmount()))
                            .reduce(BigDecimal.ZERO, BigDecimal::add)
                    )
                    .description(newOrder.getDescription())
                    .receiptUrl("https://pay.yandex.ru/transaction/" + orderSequence.get())
                    .kind("pay")
                    .currency(newOrder.getItems().get(0).getCurrency())
                    .payStatus(Order.Status.created.getCode())
                    .closed(null)
                    .created(Instant.now())
                    .caption(newOrder.getCaption())
                    .revision(1)
                    .uid(Long.valueOf(TestAuthorizationService.UID))
                    .userDescription(newOrder.getUserDescription())
                    .updated(Instant.now())
                    .build();
            merchantOrders.computeIfAbsent(serviceMerchantId, it -> new ArrayList<>())
                    .add(order);
            return order.toOrder();
        } else {
            throw new YaPayClientException("wrong merchant id:" + serviceMerchantId);
        }
    }

    @Nonnull
    private OrderRecord getOrderRecord(long merchantId, Long orderId) {
        List<OrderRecord> orders = merchantOrders.getOrDefault(merchantId, Collections.emptyList());
        return orders.stream()
                .filter(order -> order.getOrderId() == orderId)
                .findFirst()
                .orElseThrow(() -> new YaPayClientException("order not found"));

    }

    @Override
    public Order getOrder(long serviceMerchantId, long orderId) {
        return getOrderRecord(serviceMerchantId, orderId).toOrder();
    }

    @Override
    public StartOrderResponse startOrder(long serviceMerchantId, long orderId, StartOrderRequest startOrder) {
        OrderRecord order = getOrderRecord(serviceMerchantId, orderId);
        order.setPayStatus(Order.Status.held.getCode());
        order.setActive(true);
        tasks.add(executor.submit(() -> executeCallbackTask(order)));
        return new StartOrderResponse("https://trust.yandex.ru/" + order.getOrderId());
    }

    @Override
    public void clearOrder(long serviceMerchantId, long orderId) {
        OrderRecord order = getOrderRecord(serviceMerchantId, orderId);
        order.setPayStatus(Order.Status.paid.getCode());
        tasks.add(executor.submit(() -> executeCallbackTask(order)));
    }

    @Override
    public void unholdOrder(long serviceMerchantId, long orderId) {
        OrderRecord order = getOrderRecord(serviceMerchantId, orderId);
        order.setPayStatus(Order.Status.canceled.getCode());
        tasks.add(executor.submit(() -> executeCallbackTask(order)));
    }

    @Override
    public ServiceMerchantInfo requestMerchantAccess(String token, String entityId, String description)
            throws AccessRequestConflictException {
        if (TEST_MERCHANT_KEY.equals(token)) {
            if (accessRequested) {
                throw new AccessRequestConflictException(TEST_MERCHANT_ID);
            } else {
                accessRequested = true;
                return TEST_MERCHANT_INFO;
            }
        }
        throw new YaPayClientException("WRONG TOKEN");
    }

    @Override
    public ServiceMerchantInfo merchantInfo(long serviceMerchantId) {
        if (serviceMerchantId == TEST_MERCHANT_ID) {
            return TEST_MERCHANT_INFO;
        }
        throw new YaPayClientException("merchant not found");
    }

    public void clear() {
        executor.shutdownNow();
        merchantOrders.clear();
        orderSequence.set(1);
        accessRequested = false;
        queue.clear();
        tasks.clear();
    }

    public OrderRecord getOrderRecord(String purchaseToken) {
        String[] split = purchaseToken.split("\\|");
        return getOrderRecord(Long.valueOf(split[0]), Long.valueOf(split[1]));
    }

    public String executeCallback(OrderRecord order) {
        try {
            Future<String> submit = executor.submit(() -> executeCallbackTask(order));
            tasks.add(submit);
            return submit.get();
        } catch (ExecutionException | InterruptedException e) {
            throw new RuntimeException(e);
        }
    }

    public void waitCompleted() {
        new ArrayList<>(tasks).forEach(it -> {
            try {
                it.get();
            } catch (InterruptedException | ExecutionException e) {
                logger.error(e.getMessage(), e);
            }
        });
    }

    private String executeCallbackTask(OrderRecord order) {
        try {
            Thread.sleep(100);
            var request = SkillPurchaseCallbackRequest.builder()
                    .type(ORDER_STATUS_UPDATED)
                    .data(SkillPurchaseCallbackRequest.Payload.builder()
                            .serviceMerchantId(order.getServiceMerchantId())
                            .orderId(order.getOrderId())
                            .newStatus(order.getPayStatus())
                            //.updated(order.getUpdated())
                            .build())
                    .build();

            var headers = new HttpHeaders();
            headers.add(TvmHeaders.SERVICE_TICKET_HEADER, TestTvmClientImpl.getTestTicket(TvmClientName.ya_pay));

            String url = "http://localhost:" + port + "/billing/skill_purchase_callback";

            return restTemplate.postForObject(url, new HttpEntity<>(request, headers), SimpleResult.class).getResult();
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }
}
