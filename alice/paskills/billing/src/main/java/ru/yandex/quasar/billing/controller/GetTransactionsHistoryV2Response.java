package ru.yandex.quasar.billing.controller;

import java.util.List;
import java.util.stream.Collectors;

import lombok.Data;

import ru.yandex.quasar.billing.services.TransactionInfoV2;

import static ru.yandex.quasar.billing.controller.TransactionItem.getTransactionItem;

@Data
public class GetTransactionsHistoryV2Response {
    private final List<TransactionItem> items;

    public static GetTransactionsHistoryV2Response from(List<TransactionInfoV2> purchases) {
        return new GetTransactionsHistoryV2Response(
                purchases.stream()
                        .map(getTransactionItem())
                        .collect(Collectors.toList())
        );
    }
}
