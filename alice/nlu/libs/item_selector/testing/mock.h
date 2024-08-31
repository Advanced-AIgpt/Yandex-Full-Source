#pragma once

#include <alice/nlu/libs/item_selector/interface/item_selector.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>

namespace NAlice {
namespace NItemSelector {

class TMockItemSelector : public IItemSelector {
public:
    MOCK_METHOD(TVector<TSelectionResult>, Select, (const TSelectionRequest& request, const TVector<TSelectionItem>& items), (const, override));
};

} // namespace NItemSelector
} // namespace NAlice
