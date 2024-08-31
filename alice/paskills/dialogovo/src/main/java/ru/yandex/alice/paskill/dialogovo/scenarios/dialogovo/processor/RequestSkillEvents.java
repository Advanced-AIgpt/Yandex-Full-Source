package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor;

import java.util.Optional;

import lombok.Builder;
import lombok.Data;

import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.geolocation.GeolocationSharingAllowedEvent;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.geolocation.GeolocationSharingRejectedEvent;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.payment.PurchaseCompleteEvent;

/**
 * Класс служит для передачи событий от процессоров в класс {@link MegaMindRequestSkillApplier} для дальнейшей
 * обработки и отправки в webhook. До введения этого класса, логика того что нужно сделать при том или ином
 * событии помещалась внутрь этого класса (например, события аудиоплеера и активации музыкальных игрушек).
 * Это приводит к тому что логика определённого события добавляется в {@link MegaMindRequestSkillApplier},
 * хотя может быть вынесена в какой-то конкретный процессор. Это класс призван решить данную проблему.
 *
 * TODO перевести активации игрушек из {@link MegaMindRequestSkillApplier#getMusicSkillProductActivationEvent}
 * в процессор MusicSkillProductActivationProcessor
 * TODO перевести события аудиоплейра
 */
@Data
@Builder
public class RequestSkillEvents {
    @Builder.Default
    private final Optional<GeolocationSharingAllowedEvent> geolocationSharingAllowedEvent = Optional.empty();
    @Builder.Default
    private final Optional<GeolocationSharingRejectedEvent> geolocationSharingRejectedEvent = Optional.empty();
    @Builder.Default
    private final Optional<PurchaseCompleteEvent> purchaseCompleteEvent = Optional.empty();
}
