#pragma once

#include "base_handler.h"

#include <alice/bass/forms/crmbot/forms.h>

namespace NBASS::NCrmbot {

class TSearchHandler : public TCrmbotFormHandler {
public:
    TResultValue Do(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);
private:
    using EForm = ESearchForm;
    using EIntent = ESearchIntent;

    TString MakeSearchUrl(TMarketContext& ctx) const;
    TString BuildSearchQueryText(TMarketContext& ctx) const;
};

}

