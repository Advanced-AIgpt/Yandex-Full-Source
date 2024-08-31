#pragma once

#include "request_context.h"

#include <alice/personal_cards/application.h>
#include <alice/personal_cards/http_request.h>

namespace NPersonalCards {

using TServerHandler = bool (TServer::*)(const TRequestContext&, NJson::TJsonMap*);

void RegisterCardsHttpHandlers(THttpHandlersMap* handlers);

} // namespace NPersonalCards
