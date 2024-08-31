#include <alice/nlu/libs/item_selector/composite/composite.h>
#include <alice/nlu/libs/item_selector/interface/item_selector.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NItemSelector {
    namespace {
        class TPositionItemSelector : public IItemSelector {
        public:
            explicit TPositionItemSelector(const size_t position)
                : Position(position)
            {}

            TVector<TSelectionResult> Select(const TSelectionRequest& /* request */, const TVector<TSelectionItem>& items) const {
                TVector<TSelectionResult> result(items.size());

                if (Position < items.size()) {
                    result[Position].IsSelected = true;
                }

                return result;
            }
        private:
            size_t Position;
        };

        bool IsPositionSelected(const TVector<TSelectionResult>& result, const size_t position) {
            if (result.size() <= position) {
                return false;
            }

            for (size_t idx = 0; idx < result.size(); ++idx) {
                if (result[idx].IsSelected != (idx == position)) {
                    return false;
                }
            }

            return true;
        }

    } // anonymous namespace

    Y_UNIT_TEST_SUITE(TCompositeItemSelectorTestSuite) {
        Y_UNIT_TEST(TestFlow) {
            TSelectorCollection selectorCollection;

            selectorCollection["video"]["1"] = MakeHolder<TPositionItemSelector>(1);
            selectorCollection["video"]["2"] = MakeHolder<TPositionItemSelector>(2);

            selectorCollection["audio"]["3"] = MakeHolder<TPositionItemSelector>(3);
            selectorCollection["audio"]["4"] = MakeHolder<TPositionItemSelector>(4);


            TCompositeItemSelector composite {
                std::move(selectorCollection),
                MakeHolder<TPositionItemSelector>(0)
            };

            const TVector<TSelectionItem> items(10);

            TSelectionRequest request;
            UNIT_ASSERT(IsPositionSelected(composite.Select(request, items), 0));

            request.EntityType = "video";
            request.SelectorName = Nothing();
            UNIT_ASSERT(IsPositionSelected(composite.Select(request, items), 0));

            request.EntityType = "audio";
            request.SelectorName = Nothing();
            UNIT_ASSERT(IsPositionSelected(composite.Select(request, items), 0));

            request.EntityType = Nothing();
            request.SelectorName = "1";
            UNIT_ASSERT(IsPositionSelected(composite.Select(request, items), 0));

            request.EntityType = Nothing();
            request.SelectorName = "4";
            UNIT_ASSERT(IsPositionSelected(composite.Select(request, items), 0));

            request.EntityType = "video";
            request.SelectorName = "4";
            UNIT_ASSERT(IsPositionSelected(composite.Select(request, items), 0));

            request.EntityType = "audio";
            request.SelectorName = "2";
            UNIT_ASSERT(IsPositionSelected(composite.Select(request, items), 0));

            request.EntityType = "video";
            request.SelectorName = "1";
            UNIT_ASSERT(IsPositionSelected(composite.Select(request, items), 1));

            request.EntityType = "video";
            request.SelectorName = "2";
            UNIT_ASSERT(IsPositionSelected(composite.Select(request, items), 2));

            request.EntityType = "audio";
            request.SelectorName = "3";
            UNIT_ASSERT(IsPositionSelected(composite.Select(request, items), 3));

            request.EntityType = "audio";
            request.SelectorName = "4";
            UNIT_ASSERT(IsPositionSelected(composite.Select(request, items), 4));
        }
    }
}





