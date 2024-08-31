#pragma once
#include "base.h"

#include <alice/hollywood/library/scenarios/search/context/context.h>
#include <alice/hollywood/library/scenarios/search/utils/serp_helpers.h>

namespace NAlice::NHollywood::NSearch {

class TSearchNavScenario : public TSearchScenario {
public:
    using TSearchScenario::TSearchScenario;

    bool Do(const TSearchResult& response) override;

private:
    bool AddNav(const ::google::protobuf::ListValue* docs);
    bool AddNav(const NScenarios::TWebSearchDocs* docs);
    template<typename TDoc>
    bool AddNavImpl(const NJson::TJsonValue& doc, TDoc& generic);

};

} // namespace NAlice::NHollywood
