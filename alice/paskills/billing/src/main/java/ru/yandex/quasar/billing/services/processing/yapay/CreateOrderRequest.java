package ru.yandex.quasar.billing.services.processing.yapay;

import java.util.List;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Builder;
import lombok.Data;

@Data
@Builder
public class CreateOrderRequest {
    // человекочитаемое название платежа, продавец его видит в списке платежей в админке
    @Nonnull
    private final String caption;

    // описание заказа
    @JsonInclude(JsonInclude.Include.NON_NULL)
    @Nullable
    private final String description;

    // автоматически списывать деньги после hold'a или ждать вызова /clear
    @JsonProperty("autoclear")
    private final boolean autoClear;

    // адрес для получения чеков
    @JsonProperty("user_email")
    @Nonnull
    private final String userEmail;

    // uid покупателя
    @JsonProperty("customer_uid")
    @Nonnull // required for us
    private final String customerUid;

    // описание от покупателя для продавца
    @JsonProperty("user_description")
    @JsonInclude(JsonInclude.Include.NON_NULL)
    @Nullable
    private final String userDescription;

    // url для редиректа со страницы траста
    @JsonProperty("return_url")
    @Nonnull // required for us
    private final String returnUrl;

    // id привязанной карты в трасте
    @JsonProperty("paymethod_id")
    @Nullable
    private final String paymethodId;

    // список товаров заказа
    @Nonnull
    private final List<OrderItem> items;

    //  режим платежа:
    //  - prod (по-умолчанию)
    //  - test (для создания тестового заказа - платеж успешно завершится, но деньги не спишутся)
    @Nullable
    private final Mode mode;

}
