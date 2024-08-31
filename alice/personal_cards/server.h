#pragma once

#include "request_context.h"
#include "sensors.h"

#include <alice/personal_cards/push_cards_storage/push_cards_storage.h>

namespace NPersonalCards {

// Main class for cards service.
class TServer {
public:
    TServer(TPushCardsStoragePtr pushCardsStorage);

    ~TServer() = default;

    // Gracefully shutdown.
    void Shutdown();

    // Process cards request.
    bool GetCards(const TRequestContext& context, NJson::TJsonMap* result);

    // Закрытие карточки.
    bool DismissCard(const TRequestContext& context, NJson::TJsonMap* result);

    // Запрос на добавление карточки из пуша группе пользователей.
    bool AddPushCard(const TRequestContext& context, NJson::TJsonMap* result);

    // Just log and return true
    bool LogDeprecatedRequestAndReturnNothing(const TRequestContext& context, NJson::TJsonMap*);

    bool Sensors(const TRequestContext&, NJson::TJsonMap* result);

private:
    std::atomic<bool> IsShutdown_;
    TPushCardsStoragePtr PushCardsStorage_;
};

} // namespace NPersonalCards
