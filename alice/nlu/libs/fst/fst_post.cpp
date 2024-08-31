#include "fst_post.h"

namespace NAlice {

    static bool SpecialCondition(const NSc::TValue& lhs, const NSc::TValue& rhs) {
        if (rhs.DictEmpty()) {
            return true;
        }
        auto it = rhs.GetDict().begin();
        auto minKey = it->first;
        for (++it; it != rhs.GetDict().end(); ++it) {
            minKey = std::min(minKey, it->first);
        }
        return !lhs.Has(minKey);
    }

    void TFstPost::CombineEntities(TVector<TEntity>* entities) {
        if (entities->size() <= 1u) {
            return;
        }

        for (auto cur = std::next(entities->begin()); cur != entities->end(); ++cur) {
            if (!cur->ParsedToken.Value.IsDict()) {
                continue;
            }
            auto prev = std::prev(cur);
            if (!prev->ParsedToken.Value.IsDict()) {
                continue;
            }
            if (cur->Start == prev->End && SpecialCondition(prev->ParsedToken.Value, cur->ParsedToken.Value)) {
                prev->End = cur->End;
                prev->ParsedToken.Value.MergeUpdate(cur->ParsedToken.Value);
                prev->ParsedToken.StringValue += ' ';
                prev->ParsedToken.StringValue += cur->ParsedToken.StringValue;
                entities->erase(cur);
                cur = prev;
            }
        }
    }

} // namespace NAlice
