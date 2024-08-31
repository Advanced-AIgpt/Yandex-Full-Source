package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.applyarguments

import ru.yandex.alice.kronstadt.core.applyarguments.ApplyArguments

data class PurchaseCompleteApplyArguments(
    val skillId: String,
    val purchaseOfferUuid: String,
    val initialDeviceId: String,
) : ApplyArguments
