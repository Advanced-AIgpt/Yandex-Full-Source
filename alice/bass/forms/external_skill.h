#pragma once

#include "vins.h"

namespace NBASS {

/** Handler for external skills.
 * @see https://wiki.yandex-team.ru/assistant/backend/externalskills/
 * @see https://wiki.yandex-team.ru/assistant/backend/vins-bass-console/
 */
class TExternalSkillHandler : public IHandler {
public:
    ~TExternalSkillHandler() override; // just for forward decl of TApiSkillCache

    TResultValue Do(TRequestHandler& r) override;

    static void RegisterForm(THandlersMap* handlers, IGlobalContext& globalCtx);

    static void RegisterAction(THandlersMap* handlers);
};

}
